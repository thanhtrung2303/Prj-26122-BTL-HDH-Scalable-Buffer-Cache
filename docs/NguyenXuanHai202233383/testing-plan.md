# Buffer Cache Testing Plan

Author: NguyenXuanHai202233383

## 1. Correctness

- Read the same block repeatedly and verify the returned data.
- Write data and read it back to verify consistency.
- Access multiple blocks and verify that their contents do not mix.
- Exercise cache eviction and verify that reloaded data remains correct.

## 2. Concurrency

- Run multiple processes reading different blocks concurrently.
- Run multiple processes accessing the same block.
- Check for duplicate cache entries for the same device and block.
- Check reference counts, lock handling, and buffer-list integrity.
- Run available project tests and record any failures.

## 3. Sequential Read-Ahead

- Read a sequence of consecutive blocks.
- Compare behavior with read-ahead enabled and disabled, if supported.
- Verify data correctness at the beginning and end of the sequence.
- Check whether read-ahead reduces demand reads without excessive I/O.

## 4. Performance Measurements

Record the following for each test run:

| Metric | Result |
|---|---|
| Test name | Not run yet |
| Cache hits | Not measured yet |
| Cache misses | Not measured yet |
| Elapsed time | Not measured yet |
| Correctness | Not verified yet |

Do not report estimated values as measured results.

## 5. Test Environment

Record the xv6 revision, build configuration, test command, and any
relevant emulator settings so that the test can be reproduced.

## 6. Completion Checklist

- [ ] Review the existing buffer-cache implementation.
- [ ] Identify the available project tests.
- [ ] Run correctness tests.
- [ ] Run concurrency tests.
- [ ] Evaluate sequential read-ahead.
- [ ] Record actual results and observed issues.
- [ ] Review findings with the project team.
