# Week 04 – Hai: Read-ahead Testing Evidence

## Scope

This contribution covers D03 cleanup, D04, D07, D08 and R08
configuration comparison. No Week 03 files are changed by this evidence update.

## Test provenance and configuration

| Item | Recorded value |
|---|---|
| PR base/source reference requested by reviewer | `bde8afc85922f103066e27c830fb75f0643d63cc` |
| PR branch | `hai/NguyenXuanHai202233383-docs` |
| CPU count | `CPUS=3` (Makefile default) |
| `READ_AHEAD_BLOCKS` | `2` |
| `BCACHE_SELFTEST` | `0` — disabled; self-test was not run |
| `BCACHE_K01` | `0` — disabled; K01 self-test was not run |

**SHA clarification:** the SHA above is the PR base/source reference supplied by the
reviewer. It is not the PR head commit. The previously saved C0/C1 console
summaries did not record their exact tested commit SHA or CPU count at run time.
Therefore those historical traces cannot prove that C0 and C1 were run on the
same source revision. A fresh paired run is required; follow
[`RETEST-INSTRUCTIONS.md`](RETEST-INSTRUCTIONS.md) and record the resulting
PR head SHA in both logs before treating the comparison as verified.

## D03 – Cleanup

Previously observed QEMU result:
- Shared-file test: PASS.
- Separate-file test: PASS.
- Three children and 20 rounds.
- Temporary test files were removed after execution.

A fresh terminal capture for the current PR head is still required.

## D04 – Sequential Read

Previously observed QEMU result:
- Read 16 sequential blocks.
- Verify returned data and EOF.
- Result: PASS.

A fresh terminal capture for the current PR head is still required.

## D07 – File Size and EOF

Previously observed QEMU result for file sizes 0, 1023, 1024 and 1025 bytes:
- Data checks: PASS.
- EOF checks: PASS.

A fresh terminal capture for the current PR head is still required.

## D08 – Direct and Indirect Blocks

Previously observed QEMU result:
- Read 16 sequential blocks across the direct/indirect boundary.
- Verify data, EOF and unchanged file size.
- Result: PASS.

A fresh terminal capture for the current PR head is still required.

## R08 – Historical Read-ahead Comparison

The saved historical traces used this workload:

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

Interpretation: C1 records 70 read-ahead lookups, all reported as cache hits.
This workload does not demonstrate reduced disk I/O: C1 recorded four more
demand disk reads than C0. No performance improvement is claimed.

The historical traces are summaries, not raw terminal captures, and do not
contain a verifiable source SHA/CPUS header. Replace or supplement them with
the paired fresh captures described in `RETEST-INSTRUCTIONS.md`.

## CI scope

GitHub Actions checks formatting, builds the kernel and filesystem image, and
runs the repository's `usertests` and `crash` suites on Linux. macOS runs the
build. These CI jobs do not execute the interactive D03/D04/D07/D08 programs
or the R08 comparison; those require the QEMU evidence above.

## Final configuration

Restore the tracked configuration after the paired run:

- `READ_AHEAD_ENABLED 1`
- `READ_AHEAD_BLOCKS 2`
- `BCACHE_SELFTEST 0`
- `BCACHE_K01 0`
