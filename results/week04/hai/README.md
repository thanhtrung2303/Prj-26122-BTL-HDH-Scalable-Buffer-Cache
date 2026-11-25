# Week 04 – Hai: Read-ahead Testing Evidence

## Scope

This contribution covers D03 cleanup, D04 sequential read, D07 file size and
EOF behavior, D08 direct/indirect block boundary testing, and R08 read-ahead
configuration comparison. No Week 03 files are changed by this evidence update.

## Test provenance and configuration

Both C0 and C1 were tested against the same source revision:

| Item | Recorded value |
|---|---|
| Tested source SHA (C0 and C1) | `dbfe4983133577e9106b507ab55625eede1bf702` |
| PR branch | `hai/NguyenXuanHai202233383-docs` |
| CPU count | `CPUS=3` |
| `READ_AHEAD_BLOCKS` | `2` |
| `BCACHE_SELFTEST` | `0` — disabled |
| `BCACHE_K01` | `0` — disabled |

The previously supplied SHA
`bde8afc85922f103066e27c830fb75f0643d63cc` is the historical PR base/source
reference, not the source SHA used for this fresh paired test.

C0 and C1 use the same source SHA, CPU count, and configuration except for
`READ_AHEAD_ENABLED`:

| Configuration | C0 | C1 |
|---|---:|---:|
| `READ_AHEAD_ENABLED` | `0` | `1` |
| `READ_AHEAD_BLOCKS` | `2` | `2` |
| `BCACHE_SELFTEST` | `0` | `0` |
| `BCACHE_K01` | `0` | `0` |
| `CPUS` | `3` | `3` |

Raw QEMU terminal captures:
- `r08-c0.log` — read-ahead disabled.
- `r08-c1.log` — read-ahead enabled.

Both logs include the tested source SHA, configuration metadata, test output,
and cache statistics.

## D03 – Cleanup

Fresh QEMU results:
- Shared-file test: PASS.
- Separate-file test: PASS.
- Three children and 20 rounds: PASS.
- Temporary test files were cleaned up by the test.

## D04 – Sequential Read

Fresh QEMU result:
- Read 16 sequential blocks.
- Verify returned data and EOF.
- Result: PASS.

## D07 – File Size and EOF

Fresh QEMU results for file sizes 0, 1023, 1024 and 1025 bytes:
- Data checks: PASS.
- EOF checks: PASS.

## D08 – Direct and Indirect Blocks

Fresh QEMU results:
- Direct blocks: PASS.
- First indirect block: PASS.
- 16 blocks across the direct/indirect boundary: PASS.
- EOF and unchanged file size: PASS.

## R08 – Paired Read-ahead Comparison

Workload used for both configurations:

`cache_stat reset; d04test; d07test; d08test; cache_stat`

| Metric | C0: disabled | C1: enabled |
|---|---:|---:|
| Demand lookups | 3089 | 3153 |
| Demand lookup hits | 3032 | 3096 |
| Demand lookup misses | 57 | 57 |
| Demand disk reads | 57 | 57 |
| Read-ahead lookups | 0 | 70 |
| Read-ahead lookup hits | 0 | 70 |
| Read-ahead lookup misses | 0 | 0 |
| Read-ahead skips | 0 | 0 |
| Read-ahead disk reads | 0 | 0 |
| Total lookups | 3089 | 3223 |
| Total disk reads | 57 | 57 |
| Demand hit ratio | 98% | 98% |
| Read-ahead hit ratio | N/A | 100% |

Interpretation:

C1 recorded 70 read-ahead lookups, all reported as cache hits. However,
both configurations recorded 57 total disk reads. This workload therefore
does not demonstrate reduced disk I/O or a performance improvement.
No performance gain is claimed from these results.

## CI scope

GitHub Actions checks formatting, builds the kernel and filesystem image,
and runs the repository's `usertests` and `crash` suites on Linux. macOS runs
the build. These CI jobs do not execute the interactive D03/D04/D07/D08
programs or the R08 comparison; those require the QEMU evidence above.

## Final configuration

The tracked configuration is restored to:

- `READ_AHEAD_ENABLED 1`
- `READ_AHEAD_BLOCKS 2`
- `BCACHE_SELFTEST 0`
- `BCACHE_K01 0`
