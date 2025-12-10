# Additional Edge Case Tests

This document describes the new tests (test13-test28) created to cover edge cases and potential issues not covered by the original test suite.

## Test Coverage Summary

### Memory Management & Eviction Tests

**Test 13: Multiple page writes with eviction and dirty bit handling**
- Tests dirty page eviction and disk write operations
- Verifies data integrity after pages are swapped to disk
- Configuration: 4 frames, 8 blocks

**Test 15: Second-chance algorithm behavior with mixed read/write access**
- Tests clock algorithm with different access patterns
- Verifies proper handling of read-only vs. write access
- Configuration: 4 frames, 8 blocks

**Test 18: Write protection upgrade (PROT_NONE -> PROT_READ -> PROT_WRITE)**
- Tests page protection level transitions
- Verifies correct permission upgrades during page faults
- Configuration: 4 frames, 8 blocks

**Test 20: Concurrent read/write to same page with eviction**
- Tests data consistency when pages are evicted and reloaded
- Verifies pattern integrity across evictions
- Configuration: 4 frames, 8 blocks

**Test 22: Clock algorithm with all pages having PROT_NONE**
- Tests second-chance algorithm when all pages need second chances
- Verifies proper victim selection
- Configuration: 4 frames, 8 blocks

**Test 26: Repeated eviction and reload of same page**
- Stress tests data persistence across multiple eviction cycles
- Verifies disk read/write correctness
- Configuration: 4 frames, 8 blocks

**Test 28: Access pattern that exercises clock hand wrapping**
- Tests clock hand wraparound behavior
- Verifies correct handling of various access patterns
- Configuration: 4 frames, 16 blocks

### syslog Edge Cases

**Test 14: syslog with invalid addresses**
- Tests error handling for unallocated memory regions
- Verifies EINVAL is returned for invalid addresses
- Tests boundary conditions (before/after allocated region)
- Configuration: 4 frames, 8 blocks

**Test 16: syslog with zero length**
- Tests edge case of zero-length syslog calls
- Verifies proper handling of NULL pointers with zero length
- Configuration: 4 frames, 8 blocks

**Test 17: Page boundary crossing in syslog**
- Tests syslog operations that span multiple pages
- Verifies correct data reading across page boundaries
- Configuration: 4 frames, 8 blocks

**Test 24: syslog triggering page fault and eviction**
- Tests syslog on non-present pages
- Verifies that syslog properly triggers page faults
- Configuration: 4 frames, 8 blocks

**Test 25: Large syslog spanning multiple pages with eviction**
- Tests syslog operations on large memory regions
- Verifies handling of multi-page syslog calls
- Configuration: 4 frames, 8 blocks

### Resource Exhaustion Tests

**Test 19: Rapid page allocation until disk full**
- Tests behavior when disk blocks are exhausted
- Verifies ENOSPC error is properly returned
- Configuration: 4 frames, 10 blocks (intentionally limited)

**Test 23: Multiple processes with disk block exhaustion**
- Tests resource contention between multiple processes
- Verifies proper error handling in multi-process scenarios
- Configuration: 2 frames, 6 blocks, 1 fork

### Page Boundary & Access Pattern Tests

**Test 21: Access at exact page boundary**
- Tests reads/writes at exact page boundaries
- Verifies last byte of one page and first byte of next page
- Configuration: 4 frames, 8 blocks

**Test 27: Write-after-read on same page location**
- Tests protection upgrades from read to write
- Verifies multiple writes to the same location work correctly
- Configuration: 4 frames, 8 blocks

## Potential Issues These Tests May Reveal

1. **Race conditions** in the pager when handling concurrent operations
2. **Memory leaks** in page table or frame table management
3. **Incorrect dirty bit tracking** leading to data loss
4. **Clock algorithm bugs** causing infinite loops or incorrect victim selection
5. **Protection level upgrade issues** (PROT_NONE -> PROT_READ -> PROT_WRITE)
6. **Disk block allocation/deallocation bugs**
7. **Page boundary calculation errors**
8. **Error handling gaps** in edge cases
9. **Data corruption** during eviction/reload cycles
10. **syslog implementation issues** with non-present pages

## Known Issues in Current Implementation

Based on code analysis, the following potential issues were identified:

1. **pager_syslog**: The code attempts to access `pmem` directly, but `pmem` is declared as `extern const char *pmem` in mmu.h and may not be properly initialized in pager.c

2. **Clock algorithm**: The clock hand advancement might not properly handle cases where all pages have PROT_NONE initially

3. **Error handling**: Some error paths may not properly release locks before returning

4. **Memory allocation**: The `get_process` function has linear search complexity which could be slow with many processes

## Running the Tests

To run the new tests:

```bash
cd mempager
make clean
make
cd mempager-tests
# Run individual tests
./test13
./test14
# ... etc
```

Or use the grading script if available:
```bash
./grade.sh
```

## Expected Behavior

- Tests should complete without segmentation faults
- Error codes should be properly set (EINVAL, ENOSPC)
- Data should persist correctly across evictions
- Clock algorithm should make forward progress
- Multiple processes should coexist without interfering with each other
