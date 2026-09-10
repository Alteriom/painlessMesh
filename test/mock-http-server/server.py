#!/usr/bin/env python3
"""
HTTP test point for painlessMesh gateway testing

This server is the controlled Internet destination that every layer of the
gateway test suite can point at: the desktop Catch2 tests in CI, the
simulation host, the hardware farm, and a developer's bench. It answers the
way real services do -- including the ways real services get it wrong -- and
it keeps a delivery ledger so a test can ask, independently of whatever HTTP
status the gateway saw, whether the message actually arrived.

Usage:
    python3 server.py [--port PORT] [--host HOST] [--log FILE]

Environment Variables:
    MOCK_HTTP_PORT: Port to listen on (default: 8080)
    MOCK_HTTP_HOST: Host to bind to (default: 0.0.0.0)
    MOCK_HTTP_DELAY: Response delay in seconds (default: 0)
    MOCK_HTTP_STATUS: Default HTTP status code (default: 200)

Test Endpoints:
    GET/POST /status/{code}     - Return specific HTTP status code
    GET/POST /delay/{seconds}   - Respond after delay
    GET/POST /timeout           - Never respond (for timeout testing)
    GET/POST /echo              - Echo request details back
    GET/POST /whatsapp          - Simple WhatsApp-style success response
    GET      /callmebot/whatsapp.php
                                - CallMeBot emulation, see PROFILES below
    GET      /requests/{tag}    - Delivery ledger: what the service saw for a tag
    GET/POST /health            - Health check endpoint
    GET/POST /                  - Default success response

Delivery ledger:
    Every request is recorded under a tag: the ``tag`` query parameter, else
    the ``X-HIL-Tag`` header, else (for the CallMeBot route) the message text.
    ``GET /requests/{tag}`` returns the latest record for that tag, including a
    ``delivered`` boolean that says whether the emulated service accepted the
    message. A gateway test compares its own success verdict with that field.
    The record shape is a superset of the Alteriom farm's gateway probe, so a
    farm test can run against either server unchanged.
"""

import argparse
import json
import os
import sys
import threading
import time
from datetime import datetime, timezone
from http.server import BaseHTTPRequestHandler, HTTPServer
from pathlib import Path
from socketserver import ThreadingMixIn
from urllib.parse import parse_qs, urlparse


# ---------------------------------------------------------------------------
# CallMeBot emulation
#
# CallMeBot's WhatsApp API does not encode delivery in its HTTP status. Probed
# on 2026-09-10 (painlessMesh issue #450):
#   * a bogus phone/apikey answered HTTP 203 with an HTML "Oops! Too many
#     requests" page -- nothing was delivered;
#   * a request missing the apikey, or the phone, answered HTTP 201 with the
#     same error page -- nothing was delivered either;
#   * the reporter's bridge received HTTP 208, body unknown, because the
#     gateway discards response bodies.
# The success text below is the one CallMeBot's own examples and every
# published integration show for a queued message.
#
# The profile is selected by the ``apikey`` query parameter, so a test picks
# the service behaviour it wants to face without any server-side state.
# ``delivered`` is the ground truth the ledger reports for that request.
# ---------------------------------------------------------------------------

CALLMEBOT_QUEUED = (
    "<p><b>Message queued.</b> You will receive it within a few seconds.</p>"
)
CALLMEBOT_TOO_MANY = (
    "<h1>Oops! Too many requests...</h1>"
    "<p>You have called to the API to often. Please review your script/code/app.</p>"
)

CALLMEBOT_PROFILES = {
    # Observed or documented behaviour.
    "queued": {"status": 200, "body": CALLMEBOT_QUEUED, "delivered": True,
               "note": "documented success"},
    "ratelimit-203": {"status": 203, "body": CALLMEBOT_TOO_MANY, "delivered": False,
                      "note": "observed 2026-09-10 for a bogus phone/apikey"},
    "ratelimit-201": {"status": 201, "body": CALLMEBOT_TOO_MANY, "delivered": False,
                      "note": "observed 2026-09-10 for a request missing apikey or phone"},
    # The two readings of the HTTP 208 in issue #450. A gateway that keeps
    # the body can tell them apart; one that only keeps the status cannot.
    "queued-208": {"status": 208, "body": CALLMEBOT_QUEUED, "delivered": True,
                   "note": "hypothesis: the reporter's message was delivered"},
    "error-208": {"status": 208, "body": CALLMEBOT_TOO_MANY, "delivered": False,
                  "note": "hypothesis: the reporter's message was refused"},
}


class Ledger:
    """Thread-safe record of every request the test point served."""

    def __init__(self, path=None):
        self.path = Path(path) if path else None
        self.lock = threading.Lock()
        self.latest = {}
        self.count = 0

    def append(self, record):
        with self.lock:
            self.count += 1
            tag = record.get("tag")
            if tag:
                self.latest[tag] = record
            if self.path is not None:
                self.path.parent.mkdir(parents=True, exist_ok=True)
                with self.path.open("a", encoding="utf-8") as stream:
                    stream.write(json.dumps(record, sort_keys=True) + "\n")

    def get(self, tag):
        with self.lock:
            return self.latest.get(tag)


ledger = Ledger()
server_start_time = time.time()


class MockHTTPHandler(BaseHTTPRequestHandler):
    """HTTP request handler for the test point"""

    server_version = "painlessMesh-TestPoint/2"

    # Suppress default logging to reduce noise
    def log_message(self, format, *args):
        """Override to provide cleaner logging"""
        timestamp = self.log_date_time_string()
        print(f"[{timestamp}] {self.address_string()} - {format % args}")

    def _send_response(self, status_code, content_type="application/json", body=None,
                       delivered=None):
        """Send HTTP response with given status code and body"""
        self.send_response(status_code)
        self.send_header("Content-Type", content_type)
        self.send_header("Access-Control-Allow-Origin", "*")
        if delivered is not None:
            # For humans reading a capture. Tests must use the ledger: a
            # gateway that only forwards the status never sees this header.
            self.send_header("X-TestPoint-Delivered", "true" if delivered else "false")

        if body is None:
            body = json.dumps({
                "status": status_code,
                "message": self.responses.get(status_code, ["Unknown"])[0],
                "timestamp": time.time()
            })

        body_bytes = body.encode("utf-8")
        self.send_header("Content-Length", str(len(body_bytes)))
        self.end_headers()
        self.wfile.write(body_bytes)

    def _parse_path(self):
        """Parse request path and query parameters"""
        parsed = urlparse(self.path)
        path_parts = parsed.path.strip("/").split("/")
        query_params = parse_qs(parsed.query)
        return path_parts, query_params

    def _read_body(self):
        """Read request body if present"""
        content_length = int(self.headers.get("Content-Length", 0))
        if content_length > 0:
            return self.rfile.read(content_length).decode("utf-8", errors="replace")
        return ""

    def _record(self, method, query_params, body, status, delivered, extra=None):
        """Write one ledger entry for the request being served"""
        parsed = urlparse(self.path)
        tag = query_params.get("tag", [self.headers.get("X-HIL-Tag", "")])[0]
        if not tag and extra and extra.get("text"):
            tag = extra["text"]
        record = {
            "ts": datetime.now(timezone.utc).isoformat(),
            "method": method,
            "path": parsed.path,
            "tag": tag,
            "body": body or "",
            "client": self.client_address[0],
            "status": status,
            "delivered": bool(delivered),
        }
        if extra:
            record.update(extra)
        ledger.append(record)
        return record

    def do_GET(self):
        """Handle GET requests"""
        self._handle_request("GET")

    def do_POST(self):
        """Handle POST requests"""
        self._handle_request("POST")

    def do_OPTIONS(self):
        """Handle OPTIONS requests for CORS"""
        self.send_response(200)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type, X-HIL-Tag")
        self.end_headers()

    def _handle_request(self, method):
        """Handle HTTP request based on path"""
        path_parts, query_params = self._parse_path()
        body = self._read_body() if method == "POST" else None

        # Apply default delay if configured
        default_delay = float(os.getenv("MOCK_HTTP_DELAY", "0"))
        if default_delay > 0:
            time.sleep(default_delay)

        # Route to appropriate handler
        if not path_parts or path_parts[0] == "":
            self._handle_default(method, query_params, body)
        elif path_parts[0] == "status" and len(path_parts) > 1:
            self._handle_status(path_parts[1], method, query_params, body)
        elif path_parts[0] == "delay" and len(path_parts) > 1:
            self._handle_delay(path_parts[1], method, query_params, body)
        elif path_parts[0] == "timeout":
            self._handle_timeout(method, query_params, body)
        elif path_parts[0] == "echo":
            self._handle_echo(method, query_params, body)
        elif path_parts[0] == "whatsapp":
            self._handle_whatsapp(method, query_params, body)
        elif path_parts[0] == "callmebot":
            self._handle_callmebot(method, path_parts[1:], query_params)
        elif path_parts[0] == "requests":
            self._handle_requests(path_parts[1:])
        elif path_parts[0] == "health":
            self._handle_health()
        else:
            self._handle_not_found()

    def _handle_default(self, method, query_params, body):
        """Handle default endpoint"""
        status = int(os.getenv("MOCK_HTTP_STATUS", "200"))
        self._record(method, query_params, body, status, 200 <= status < 300)
        response = {
            "message": "painlessMesh HTTP test point",
            "status": "ok",
            "timestamp": time.time()
        }
        self._send_response(status, body=json.dumps(response))

    def _handle_status(self, code_str, method, query_params, body):
        """Handle /status/{code} endpoint"""
        try:
            status_code = int(code_str)
            if status_code < 100 or status_code > 599:
                raise ValueError("Invalid status code")

            delivered = 200 <= status_code < 300
            record = self._record(method, query_params, body, status_code, delivered)
            response = {
                "ok": delivered,
                "requested_status": status_code,
                "message": f"Mock response with status {status_code}",
                "request": record,
                "timestamp": time.time()
            }
            self._send_response(status_code, body=json.dumps(response), delivered=delivered)
        except ValueError:
            self._send_response(400, body=json.dumps({
                "error": "Invalid status code",
                "provided": code_str
            }))

    def _handle_delay(self, delay_str, method, query_params, body):
        """Handle /delay/{seconds} endpoint"""
        try:
            delay = float(delay_str)
            if delay < 0 or delay > 300:  # Max 5 minutes
                raise ValueError("Delay out of range")

            # Recorded before the stall, so /requests/{tag} proves the gateway
            # issued the request even when it gives up before the reply lands.
            self._record(method, query_params, body, 200, True)
            time.sleep(delay)
            response = {
                "ok": True,
                "message": f"Response delayed by {delay} seconds",
                "delay": delay,
                "timestamp": time.time()
            }
            self._send_response(200, body=json.dumps(response), delivered=True)
        except ValueError as e:
            self._send_response(400, body=json.dumps({
                "error": f"Invalid delay: {e}",
                "provided": delay_str
            }))

    def _handle_timeout(self, method, query_params, body):
        """Handle /timeout endpoint - never responds"""
        self._record(method, query_params, body, 0, False)
        print("[INFO] Timeout endpoint called - hanging connection")
        # Nothing to catch: the client gives up long before this returns, and
        # time.sleep() has retried on signal interruption since Python 3.5, so
        # the only way out of it is the sleep expiring.
        time.sleep(3600)  # 1 hour - client will timeout first

    def _handle_echo(self, method, query_params, body):
        """Handle /echo endpoint - echo request details"""
        self._record(method, query_params, body, 200, True)
        response = {
            "method": method,
            "path": self.path,
            "headers": dict(self.headers),
            "query_params": query_params,
            "body": body,
            "client": self.client_address[0],
            "timestamp": time.time()
        }
        self._send_response(200, body=json.dumps(response, indent=2), delivered=True)

    def _handle_whatsapp(self, method, query_params, body):
        """Handle /whatsapp endpoint - a well-behaved WhatsApp-style API"""
        phone = query_params.get("phone", [None])[0]
        apikey = query_params.get("apikey", [None])[0]
        text = query_params.get("text", [None])[0]

        if not phone or not apikey or not text:
            self._record(method, query_params, body, 400, False)
            response = {
                "error": "Missing required parameters",
                "required": ["phone", "apikey", "text"],
                "provided": list(query_params.keys())
            }
            self._send_response(400, body=json.dumps(response), delivered=False)
            return

        self._record(method, query_params, body, 200, True, {"text": text})
        response = {
            "message": "WhatsApp message queued successfully",
            "phone": phone,
            "text": text[:50] + "..." if len(text) > 50 else text,
            "timestamp": time.time()
        }
        self._send_response(200, body=json.dumps(response), delivered=True)

    def _handle_callmebot(self, method, subpath, query_params):
        """Handle /callmebot/whatsapp.php - emulate CallMeBot, profile by apikey"""
        if subpath != ["whatsapp.php"] or method != "GET":
            self._send_response(404, body=json.dumps({
                "error": "CallMeBot emulation serves GET /callmebot/whatsapp.php only",
                "profiles": sorted(CALLMEBOT_PROFILES),
            }))
            return

        phone = query_params.get("phone", [None])[0]
        apikey = query_params.get("apikey", [None])[0]
        text = query_params.get("text", [None])[0]
        profile = CALLMEBOT_PROFILES.get(apikey or "")

        if profile is None or not phone or not text:
            # A typo in a test must fail loudly, not look like a service quirk.
            self._record(method, query_params, None, 400, False,
                         {"text": text or "", "profile": apikey or ""})
            self._send_response(400, body=json.dumps({
                "error": "unknown CallMeBot profile or missing phone/text",
                "apikey": apikey,
                "profiles": {k: v["note"] for k, v in CALLMEBOT_PROFILES.items()},
            }), delivered=False)
            return

        self._record(method, query_params, None, profile["status"], profile["delivered"],
                     {"text": text, "profile": apikey, "response": profile["body"]})
        self._send_response(profile["status"], content_type="text/html; charset=utf-8",
                            body=profile["body"], delivered=profile["delivered"])

    def _handle_requests(self, subpath):
        """Handle /requests/{tag} - read the delivery ledger"""
        tag = "/".join(subpath)
        record = ledger.get(tag) if tag else None
        if record is None:
            self._send_response(404, body=json.dumps({"error": "not found", "tag": tag}))
            return
        self._send_response(200, body=json.dumps(record, sort_keys=True))

    def _handle_health(self):
        """Handle /health endpoint"""
        response = {
            "status": "healthy",
            "ok": True,
            "server": "painlessMesh-TestPoint",
            "version": "2.0.0",
            "uptime": time.time() - server_start_time,
            "requests_seen": ledger.count,
            "timestamp": time.time()
        }
        self._send_response(200, body=json.dumps(response))

    def _handle_not_found(self):
        """Handle 404 Not Found"""
        response = {
            "error": "Endpoint not found",
            "path": self.path,
            "available_endpoints": [
                "/",
                "/status/{code}",
                "/delay/{seconds}",
                "/timeout",
                "/echo",
                "/whatsapp",
                "/callmebot/whatsapp.php",
                "/requests/{tag}",
                "/health"
            ]
        }
        self._send_response(404, body=json.dumps(response))


class ThreadedHTTPServer(ThreadingMixIn, HTTPServer):
    """Handle requests in separate threads"""
    daemon_threads = True
    allow_reuse_address = True


def main():
    """Main entry point"""
    global ledger, server_start_time
    server_start_time = time.time()

    parser = argparse.ArgumentParser(
        description="HTTP test point for painlessMesh gateway testing"
    )
    parser.add_argument(
        "--port",
        type=int,
        default=int(os.getenv("MOCK_HTTP_PORT", "8080")),
        help="Port to listen on (default: 8080)"
    )
    parser.add_argument(
        "--host",
        default=os.getenv("MOCK_HTTP_HOST", "0.0.0.0"),
        help="Host to bind to (default: 0.0.0.0)"
    )
    parser.add_argument(
        "--log",
        default=os.getenv("MOCK_HTTP_LOG", ""),
        help="Append every ledger record as JSON lines to this file (default: memory only)"
    )
    args = parser.parse_args()

    ledger = Ledger(args.log or None)
    server = ThreadedHTTPServer((args.host, args.port), MockHTTPHandler)

    print("=" * 60)
    print("painlessMesh HTTP test point")
    print("=" * 60)
    print(f"Listening on: http://{args.host}:{args.port}")
    print(f"Start time: {time.strftime('%Y-%m-%d %H:%M:%S')}")
    if args.log:
        print(f"Ledger log: {args.log}")
    print()
    print("Available endpoints:")
    print("  GET/POST /                        - Default success response")
    print("  GET/POST /status/{code}           - Return specific HTTP status")
    print("  GET/POST /delay/{seconds}         - Delayed response")
    print("  GET/POST /timeout                 - Never responds (timeout test)")
    print("  GET/POST /echo                    - Echo request details")
    print("  GET/POST /whatsapp                - Well-behaved WhatsApp-style API")
    print("  GET      /callmebot/whatsapp.php  - CallMeBot emulation (apikey = profile)")
    print("  GET      /requests/{tag}          - Delivery ledger")
    print("  GET/POST /health                  - Health check")
    print()
    print("CallMeBot profiles:")
    for name, profile in CALLMEBOT_PROFILES.items():
        print(f"  {name:14s} HTTP {profile['status']}  delivered={profile['delivered']!s:5}  {profile['note']}")
    print()
    print("Press Ctrl+C to stop")
    print("=" * 60)
    print()

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n\nShutting down server...")
        server.shutdown()
        print("Server stopped.")
        sys.exit(0)


if __name__ == "__main__":
    main()
