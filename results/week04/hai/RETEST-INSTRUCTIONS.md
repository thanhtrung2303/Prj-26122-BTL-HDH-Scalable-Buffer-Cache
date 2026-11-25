# Week 04 – paired QEMU retest instructions

Run these steps from the repository checkout after the formatting/CI-fix commits
have been pushed to PR #21. Do not switch commits between C0 and C1.

## 1. Record the exact source revision

```sh
git status --short
git rev-parse HEAD
grep -E '^(#define (READ_AHEAD_ENABLED|READ_AHEAD_BLOCKS|BCACHE_SELFTEST|BCACHE_K01))' kernel/param.h
grep -n '^CPUS' Makefile
```

Use the exact `git rev-parse HEAD` output as the tested source SHA in **both**
C0 and C1 records. The Makefile default is CPUS=3. Keep all source files
unchanged during the pair; only toggle `READ_AHEAD_ENABLED`.

## 2. C0 – read-ahead disabled

```sh
sed -i 's/^#define READ_AHEAD_ENABLED 1$/#define READ_AHEAD_ENABLED 0/' kernel/param.h
make clean
make
make qemu CPUS=3
```

At the xv6 shell, run:

```text
d03test
d04test
d07test
d08test
cache_stat reset
d04test
d07test
d08test
cache_stat
```

Capture the complete QEMU terminal session as `r08-c0.log` (include the
host-side source SHA and config header). Exit QEMU with Ctrl-A, then X.

## 3. C1 – read-ahead enabled

Without changing branches or commits:

```sh
sed -i 's/^#define READ_AHEAD_ENABLED 0$/#define READ_AHEAD_ENABLED 1/' kernel/param.h
make clean
make
make qemu CPUS=3
```

Run the same xv6 commands in the same order. Capture as `r08-c1.log`.
The source SHA, CPUS, block count, test order and workload must match C0.

## 4. Restore the tracked configuration

After exiting QEMU:

```sh
sed -i 's/^#define READ_AHEAD_ENABLED 1$/#define READ_AHEAD_ENABLED 1/' kernel/param.h
grep -E '^(#define (READ_AHEAD_ENABLED|READ_AHEAD_BLOCKS|BCACHE_SELFTEST|BCACHE_K01))' kernel/param.h
git diff -- kernel/param.h
```

Expected final values: `READ_AHEAD_ENABLED=1`, `READ_AHEAD_BLOCKS=2`,
`BCACHE_SELFTEST=0`, `BCACHE_K01=0`. The final diff for `kernel/param.h`
should be empty.

## 5. What to save

For each configuration, keep:
- Full terminal log, not a manually rewritten summary.
- Source SHA from step 1, CPUS=3, config values, and QEMU version.
- D03/D04/D07/D08 PASS output and R08 statistics.
- A screenshot showing the PASS output and the configuration label.

Do not state that the paired comparison is complete if either configuration
fails or if the source SHA/configuration differs.
