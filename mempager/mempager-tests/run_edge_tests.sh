#!/bin/bash
# Quick test runner for new edge case tests

echo "=========================================="
echo "Running Edge Case Tests (test13-test30)"
echo "=========================================="
echo ""

TESTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$TESTS_DIR"

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

passed=0
failed=0
total=0

for i in {13..30}; do
    test_name="test${i}"
    
    if [ ! -f "${test_name}.c" ]; then
        continue
    fi
    
    total=$((total + 1))
    echo -n "Running ${test_name}... "
    
    # Check if test exists in spec file
    if ! grep -q "^${i} " tests.spec; then
        echo -e "${YELLOW}SKIP${NC} (not in tests.spec)"
        continue
    fi
    
    # Run the test if executable exists
    if [ -f "./${test_name}" ]; then
        if timeout 5s ./${test_name} > /dev/null 2>&1; then
            echo -e "${GREEN}PASS${NC}"
            passed=$((passed + 1))
        else
            exit_code=$?
            echo -e "${RED}FAIL${NC} (exit code: $exit_code)"
            failed=$((failed + 1))
        fi
    else
        echo -e "${YELLOW}SKIP${NC} (not compiled)"
    fi
done

echo ""
echo "=========================================="
echo "Test Summary"
echo "=========================================="
echo "Total:  $total"
echo -e "Passed: ${GREEN}$passed${NC}"
echo -e "Failed: ${RED}$failed${NC}"
echo "=========================================="

if [ $failed -eq 0 ]; then
    exit 0
else
    exit 1
fi
