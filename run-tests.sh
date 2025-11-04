#!/bin/bash
set -e

echo "================================="
echo "painlessMesh Build & Test System"
echo "================================="
echo ""
echo "🔧 Configuring build..."
cmake -G Ninja .

echo ""
echo "🔨 Building tests..."
ninja

echo ""
echo "🧪 Running tests..."
echo ""

# Track test results
TESTS_PASSED=0
TESTS_FAILED=0

for test in bin/catch_*; do
  if [ -x "$test" ]; then
    echo "▶️  Running: $(basename $test)"
    echo "-----------------------------------"
    if $test; then
      echo "✅ $(basename $test) PASSED"
      ((TESTS_PASSED++))
    else
      echo "❌ $(basename $test) FAILED"
      ((TESTS_FAILED++))
    fi
    echo ""
  fi
done

echo "================================="
if [ $TESTS_FAILED -eq 0 ]; then
  echo "✅ All tests passed! ($TESTS_PASSED/$((TESTS_PASSED + TESTS_FAILED)))"
  echo "================================="
  exit 0
else
  echo "❌ $TESTS_FAILED test(s) failed! ($TESTS_PASSED passed)"
  echo "================================="
  exit 1
fi
