#!/bin/bash
# Test script for mock HTTP server
# This demonstrates all available endpoints

set -e

SERVER_URL="${1:-http://localhost:8080}"
PASS=0
FAIL=0

echo "=========================================="
echo "Mock HTTP Server Test Suite"
echo "=========================================="
echo "Testing server at: $SERVER_URL"
echo ""

# Color codes
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

test_endpoint() {
    local name="$1"
    local url="$2"
    local expected_status="${3:-200}"
    local method="${4:-GET}"
    
    echo -n "Testing $name... "
    
    if [ "$method" = "POST" ]; then
        response=$(curl -s -w "\n%{http_code}" -X POST "$url" -H "Content-Type: application/json" -d '{"test":"data"}' || echo "000")
    else
        response=$(curl -s -w "\n%{http_code}" "$url" || echo "000")
    fi
    
    status_code=$(echo "$response" | tail -n 1)
    body=$(echo "$response" | head -n -1)
    
    if [ "$status_code" = "$expected_status" ]; then
        echo -e "${GREEN}✓ PASS${NC} (HTTP $status_code)"
        PASS=$((PASS+1))
        return 0
    else
        echo -e "${RED}✗ FAIL${NC} (Expected $expected_status, got $status_code)"
        echo "Response: $body"
        FAIL=$((FAIL+1))
        return 1
    fi
}

# Health check
test_endpoint "Health check" "$SERVER_URL/health" 200

# Default endpoint
test_endpoint "Default endpoint" "$SERVER_URL/" 200

# Status endpoints
test_endpoint "Status 200" "$SERVER_URL/status/200" 200
test_endpoint "Status 201" "$SERVER_URL/status/201" 201
test_endpoint "Status 203" "$SERVER_URL/status/203" 203

# CallMeBot emulation: status by profile, and the ledger's delivery verdict
test_endpoint "CallMeBot queued" "$SERVER_URL/callmebot/whatsapp.php?phone=%2B1&apikey=queued&text=t-queued" 200
test_endpoint "CallMeBot ratelimit-203" "$SERVER_URL/callmebot/whatsapp.php?phone=%2B1&apikey=ratelimit-203&text=t-203" 203
test_endpoint "CallMeBot ratelimit-201" "$SERVER_URL/callmebot/whatsapp.php?phone=%2B1&apikey=ratelimit-201&text=t-201" 201
test_endpoint "CallMeBot unverified-208" "$SERVER_URL/callmebot/whatsapp.php?phone=%2B1&apikey=unverified-208&text=t-208" 208
test_endpoint "CallMeBot queued-208" "$SERVER_URL/callmebot/whatsapp.php?phone=%2B1&apikey=queued-208&text=t-q208" 208
test_endpoint "CallMeBot paused-after-echo" "$SERVER_URL/callmebot/whatsapp.php?phone=%2B1&apikey=paused-after-echo&text=t-paused" 200
echo -n "The chunked profile is sent chunked and decodes to the queued text... "
chunked_headers=$(curl -s -D - -o /dev/null --http1.1 "$SERVER_URL/callmebot/whatsapp.php?phone=%2B1&apikey=queued-chunked&text=t-chunked-h")
chunked_body=$(curl -s --http1.1 "$SERVER_URL/callmebot/whatsapp.php?phone=%2B1&apikey=queued-chunked&text=t-chunked")
if echo "$chunked_headers" | grep -qi '^Transfer-Encoding: chunked' && [[ "$chunked_body" == *"Message queued."* ]]; then
    echo -e "${GREEN}✓ PASS${NC}"; PASS=$((PASS+1))
else
    echo -e "${RED}✗ FAIL${NC}"; FAIL=$((FAIL+1))
fi
echo -n "The paused reply is longer than a gateway keeps, verdict last... "
paused_body=$(curl -s "$SERVER_URL/callmebot/whatsapp.php?phone=%2B1&apikey=paused-after-echo&text=t-paused2")
if [ "${#paused_body}" -gt 768 ] && [[ "${paused_body: -160}" == *"Account is Paused"* ]]; then
    echo -e "${GREEN}✓ PASS${NC}"; PASS=$((PASS+1))
else
    echo -e "${RED}✗ FAIL${NC} (${#paused_body} bytes)"; FAIL=$((FAIL+1))
fi
test_endpoint "CallMeBot unknown profile" "$SERVER_URL/callmebot/whatsapp.php?phone=%2B1&apikey=nope&text=t-nope" 400
test_endpoint "Ledger record" "$SERVER_URL/requests/t-201" 200
test_endpoint "Ledger miss" "$SERVER_URL/requests/never-sent" 404
echo -n "Ledger says ratelimit-201 was not delivered... "
if curl -s "$SERVER_URL/requests/t-201" | grep -q '"delivered": false'; then
    echo -e "${GREEN}✓ PASS${NC}"; PASS=$((PASS+1))
else
    echo -e "${RED}✗ FAIL${NC}"; FAIL=$((FAIL+1))
fi
echo -n "Health names the ledger and Retry-After features... "
if curl -s "$SERVER_URL/health" | grep -q '"retry_after"'; then
    echo -e "${GREEN}✓ PASS${NC}"; PASS=$((PASS+1))
else
    echo -e "${RED}✗ FAIL${NC}"; FAIL=$((FAIL+1))
fi

# Retry-After: refused once per tag, accepted afterwards
RA_TAG="ra-$$-$(date +%s)"
test_endpoint "Retry-After first request" "$SERVER_URL/retry-after/1?tag=$RA_TAG" 429
echo -n "Retry-After header is sent... "
if curl -s -D - -o /dev/null "$SERVER_URL/retry-after/1?tag=$RA_TAG-h" | grep -qi '^Retry-After: 1'; then
    echo -e "${GREEN}✓ PASS${NC}"; PASS=$((PASS+1))
else
    echo -e "${RED}✗ FAIL${NC}"; FAIL=$((FAIL+1))
fi
test_endpoint "Retry-After second request" "$SERVER_URL/retry-after/1?tag=$RA_TAG" 200
echo -n "Ledger counts both requests and their request id... "
curl -s -H "X-Request-Id: pm-1-1" "$SERVER_URL/status/200?tag=$RA_TAG-id" > /dev/null
curl -s -H "X-Request-Id: pm-1-1" "$SERVER_URL/status/200?tag=$RA_TAG-id" > /dev/null
record=$(curl -s "$SERVER_URL/requests/$RA_TAG-id")
if echo "$record" | grep -q '"count": 2' && echo "$record" | grep -q '"request_ids": \["pm-1-1"\]'; then
    echo -e "${GREEN}✓ PASS${NC}"; PASS=$((PASS+1))
else
    echo -e "${RED}✗ FAIL${NC} ($record)"; FAIL=$((FAIL+1))
fi
test_endpoint "Status 400" "$SERVER_URL/status/400" 400
test_endpoint "Status 404" "$SERVER_URL/status/404" 404
test_endpoint "Status 500" "$SERVER_URL/status/500" 500

# Delay endpoint (1 second)
echo -n "Testing Delay endpoint (1s)... "
start=$(date +%s)
test_endpoint "Delay 1s" "$SERVER_URL/delay/1" 200 > /dev/null 2>&1
end=$(date +%s)
duration=$((end - start))
if [ $duration -ge 1 ]; then
    echo -e "${GREEN}✓ PASS${NC} (Delayed ${duration}s)"
    PASS=$((PASS+1))
else
    echo -e "${RED}✗ FAIL${NC} (Expected >= 1s delay, got ${duration}s)"
    FAIL=$((FAIL+1))
fi

# Echo endpoint
echo -n "Testing Echo endpoint... "
response=$(curl -s -X POST "$SERVER_URL/echo" \
    -H "Content-Type: application/json" \
    -d '{"sensor":"temp","value":25.5}')
if echo "$response" | grep -q '"method": "POST"' && echo "$response" | grep -q '"body":'; then
    echo -e "${GREEN}✓ PASS${NC}"
    PASS=$((PASS+1))
else
    echo -e "${RED}✗ FAIL${NC}"
    echo "Response: $response"
    FAIL=$((FAIL+1))
fi

# WhatsApp endpoint - success
echo -n "Testing WhatsApp endpoint (valid)... "
response=$(curl -s "$SERVER_URL/whatsapp?phone=%2B1234567890&apikey=test&text=Hello")
if echo "$response" | grep -q '"message": "WhatsApp message queued successfully"'; then
    echo -e "${GREEN}✓ PASS${NC}"
    PASS=$((PASS+1))
else
    echo -e "${RED}✗ FAIL${NC}"
    echo "Response: $response"
    FAIL=$((FAIL+1))
fi

# WhatsApp endpoint - missing params
echo -n "Testing WhatsApp endpoint (missing params)... "
status_code=$(curl -s -w "%{http_code}" -o /dev/null "$SERVER_URL/whatsapp?phone=%2B123")
if [ "$status_code" = "400" ]; then
    echo -e "${GREEN}✓ PASS${NC} (Correctly rejected)"
    PASS=$((PASS+1))
else
    echo -e "${RED}✗ FAIL${NC} (Expected 400, got $status_code)"
    FAIL=$((FAIL+1))
fi

# Timeout endpoint (should timeout after 5 seconds)
echo -n "Testing Timeout endpoint (5s max)... "
start=$(date +%s)
status_code=$(curl -s -w "%{http_code}" -o /dev/null --max-time 5 "$SERVER_URL/timeout" || echo "timeout")
end=$(date +%s)
duration=$((end - start))
if [ "$status_code" = "timeout" ] || [ $duration -ge 4 ]; then
    echo -e "${GREEN}✓ PASS${NC} (Timed out after ${duration}s)"
    PASS=$((PASS+1))
else
    echo -e "${RED}✗ FAIL${NC} (Should have timed out)"
    FAIL=$((FAIL+1))
fi

# Summary
echo ""
echo "=========================================="
echo "Test Results"
echo "=========================================="
echo "Total: $((PASS + FAIL))"
echo -e "${GREEN}Passed: $PASS${NC}"
if [ $FAIL -gt 0 ]; then
    echo -e "${RED}Failed: $FAIL${NC}"
else
    echo "Failed: $FAIL"
fi
echo "=========================================="

if [ $FAIL -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed!${NC}"
    exit 1
fi
