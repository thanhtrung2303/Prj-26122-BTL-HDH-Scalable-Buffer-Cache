# Week 04 – Hai: Read-ahead Testing Evidence

## Scope

This contribution covers D03 cleanup, D04, D07, D08 and R08
configuration comparison.

No Week 03 documentation or evidence is modified.

## D03 – Cleanup

- Shared-file test: PASS.
- Separate-file test: PASS.
- Temporary test files are removed after execution.
- Three children and 20 rounds were tested.

## D04 – Sequential Read

- Read 16 sequential blocks.
- Verify returned data.
- Verify EOF behavior.
- Result: PASS in C0 and C1.

## D07 – File Size and EOF

Tested file sizes:

- 0 bytes
- BSIZE - 1 (1023 bytes)
- BSIZE (1024 bytes)
- BSIZE + 1 (1025 bytes)

Data and EOF checks passed in C0 and C1.

## D08 – Direct and Indirect Blocks

- Read 16 sequential blocks.
- Verify data across the direct/indirect boundary.
- Verify EOF.
- Verify file size remains unchanged during reading.
- Result: PASS in C0 and C1.

## R08 – Read-ahead Configuration Comparison

Both configurations used the same workload:

`cache_stat reset; d04test; d07test; d08test; cache_stat`

| Metric | C0: disabled | C1: enabled |
|---|---:|---:|
| Demand lookups | 3089 | 3089 |
| Demand lookup hits | 3031 | 3027 |
| Demand lookup misses | 58 | 62 |
| Demand disk reads | 58 | 62 |
| Read-ahead lookups | 0 | 70 |
| Read-ahead lookup hits | 0 | 70 |
| Read-ahead lookup misses | 0 | 0 |
| Read-ahead skips | 0 | 0 |
| Read-ahead disk reads | 0 | 0 |
| Total lookups | 3089 | 3159 |
| Total disk reads | 58 | 62 |
| Demand hit ratio | 98% | 97% |
| Read-ahead hit ratio | N/A | 100% |

### Interpretation

C0 confirms that read-ahead lookups are disabled.

C1 records 70 read-ahead lookups, all reported as cache hits.
No read-ahead disk reads occurred in this workload.

This result confirms read-ahead lookup activity, but does not demonstrate
a reduction in disk I/O for this particular workload. C1 recorded four
more demand disk reads than C0.

## Trace Files

- `r08-c0.log`: read-ahead disabled.
- `r08-c1.log`: read-ahead enabled.

## Final Configuration

The working tree was restored to:

- `READ_AHEAD_ENABLED 1`
- `READ_AHEAD_BLOCKS 2`
