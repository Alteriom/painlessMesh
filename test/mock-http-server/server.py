#!/usr/bin/env python3
"""
Mock HTTP Server for painlessMesh Bridge Testing

This server provides a local endpoint for testing sendToInternet() functionality
without requiring actual Internet connectivity. It simulates various HTTP responses,
error conditions, and DNS resolution scenarios.

Usage:
    python3 server.py [--port PORT] [--host HOST]

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
    GET/POST /whatsapp          - Simulate Callmebot WhatsApp API
    GET/POST /health            - Health check endpoint
    GET/POST /                  - Default success response
"""

import argparse
import json
import os
import sys
import time
from http.server import BaseHTTPRequestHandler, HTTPServer
from socketserver import ThreadingMixIn
from urllib.parse import parse_qs, urlparse


class MockHTTPHandler(BaseHTTPRequestHandler):
    """HTTP request handler for mock server"""

    # Suppress default logging to reduce noise
    def log_message(self, format, *args):
        """Override to provide cleaner logging"""
        timestamp = self.log_date_time_string()
        print(f"[{timestamp}] {self.address_string()} - {format % args}")

    def _send_response(self, status_code, content_type="application/json", body=None):
        """Send HTTP response with given status code and body"""
        self.send_response(status_code)
        self.send_header("Content-Type", content_type)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Server", "painlessMesh-MockServer/1.0")
        
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
            return self.rfile.read(content_length).decode("utf-8")
        return ""

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
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
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
            self._handle_default()
        elif path_parts[0] == "status" and len(path_parts) > 1:
            self._handle_status(path_parts[1])
        elif path_parts[0] == "delay" and len(path_parts) > 1:
            self._handle_delay(path_parts[1])
        elif path_parts[0] == "timeout":
            self._handle_timeout()
        elif path_parts[0] == "echo":
            self._handle_echo(method, query_params, body)
        elif path_parts[0] == "whatsapp":
            self._handle_whatsapp(query_params)
        elif path_parts[0] == "health":
            self._handle_health()
        else:
            self._handle_not_found()

    def _handle_default(self):
        """Handle default endpoint"""
        status = int(os.getenv("MOCK_HTTP_STATUS", "200"))
        response = {
            "message": "painlessMesh Mock HTTP Server",
            "status": "ok",
            "timestamp": time.time()
        }
        self._send_response(status, body=json.dumps(response))

    def _handle_status(self, code_str):
        """Handle /status/{code} endpoint"""
        try:
            status_code = int(code_str)
            if status_code < 100 or status_code > 599:
                raise ValueError("Invalid status code")
            
            response = {
                "requested_status": status_code,
                "message": f"Mock response with status {status_code}",
                "timestamp": time.time()
            }
            self._send_response(status_code, body=json.dumps(response))
        except ValueError:
            self._send_response(400, body=json.dumps({
                "error": "Invalid status code",
                "provided": code_str
            }))

    def _handle_delay(self, delay_str):
        """Handle /delay/{seconds} endpoint"""
        try:
            delay = float(delay_str)
            if delay < 0 or delay > 300:  # Max 5 minutes
                raise ValueError("Delay out of range")
            
            time.sleep(delay)
            response = {
                "message": f"Response delayed by {delay} seconds",
                "delay": delay,
                "timestamp": time.time()
            }
            self._send_response(200, body=json.dumps(response))
        except ValueError as e:
            self._send_response(400, body=json.dumps({
                "error": f"Invalid delay: {e}",
                "provided": delay_str
            }))

    def _handle_timeout(self):
        """Handle /timeout endpoint - never responds"""
        # Simply wait indefinitely - client will timeout
        print(f"[INFO] Timeout endpoint called - hanging connection")
        try:
            time.sleep(3600)  # 1 hour - client will timeout first
        except:
            pass

    def _handle_echo(self, method, query_params, body):
        """Handle /echo endpoint - echo request details"""
        response = {
            "method": method,
            "path": self.path,
            "headers": dict(self.headers),
            "query_params": query_params,
            "body": body,
            "client": self.client_address[0],
            "timestamp": time.time()
        }
        self._send_response(200, body=json.dumps(response, indent=2))

    def _handle_whatsapp(self, query_params):
        """Handle /whatsapp endpoint - simulate Callmebot WhatsApp API"""
        # Validate required parameters
        phone = query_params.get("phone", [None])[0]
        apikey = query_params.get("apikey", [None])[0]
        text = query_params.get("text", [None])[0]

        if not phone or not apikey or not text:
            response = {
                "error": "Missing required parameters",
                "required": ["phone", "apikey", "text"],
                "provided": list(query_params.keys())
            }
            self._send_response(400, body=json.dumps(response))
            return

        # Simulate successful WhatsApp API response
        response = {
            "message": "WhatsApp message queued successfully",
            "phone": phone,
            "text": text[:50] + "..." if len(text) > 50 else text,
            "timestamp": time.time()
        }
        self._send_response(200, body=json.dumps(response))

    def _handle_health(self):
        """Handle /health endpoint"""
        response = {
            "status": "healthy",
            "server": "painlessMesh-MockServer",
            "version": "1.0.0",
            "uptime": time.time() - server_start_time,
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
    global server_start_time
    server_start_time = time.time()

    parser = argparse.ArgumentParser(
        description="Mock HTTP Server for painlessMesh Bridge Testing"
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
    args = parser.parse_args()

    server = ThreadedHTTPServer((args.host, args.port), MockHTTPHandler)
    
    print("=" * 60)
    print("painlessMesh Mock HTTP Server")
    print("=" * 60)
    print(f"Listening on: http://{args.host}:{args.port}")
    print(f"Start time: {time.strftime('%Y-%m-%d %H:%M:%S')}")
    print()
    print("Available endpoints:")
    print("  GET/POST /                  - Default success response")
    print("  GET/POST /status/{code}     - Return specific HTTP status")
    print("  GET/POST /delay/{seconds}   - Delayed response")
    print("  GET/POST /timeout           - Never responds (timeout test)")
    print("  GET/POST /echo              - Echo request details")
    print("  GET/POST /whatsapp          - Simulate Callmebot API")
    print("  GET/POST /health            - Health check")
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
