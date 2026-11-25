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

Before starting QEMU, write the provenance header to the log:

```sh
{
  echo "SOURCE_SHA=$(git rev-parse HEAD)"
  echo "CPUS=3"
  echo "READ_AHEAD_ENABLED=0"
  echo "READ_AHEAD_BLOCKS=2"
  echo "BCACHE_SELFTEST=0"
  echo "BCACHE_K01=0"
  qemu-system-riscv64 --version | head -n 1
} > results/week04/hai/r08-c0.log
script -q -a -c "make qemu CPUS=3" results/week04/hai/r08-c0.log
```

At the xv6 shell, run the commands listed above. Exit QEMU with Ctrl-A, then X.
The same terminal log contains the host header and the interactive QEMU output.

## 3. C1 – read-ahead enabled

Without changing branches or commits:

```sh
sed -i 's/^#define READ_AHEAD_ENABLED 0$/#define READ_AHEAD_ENABLED 1/' kernel/param.h
make clean
make
make qemu CPUS=3
```

Before starting QEMU, write the matching provenance header:

```sh
{
  echo "SOURCE_SHA=$(git rev-parse HEAD)"
  echo "CPUS=3"
  echo "READ_AHEAD_ENABLED=1"
  echo "READ_AHEAD_BLOCKS=2"
  echo "BCACHE_SELFTEST=0"
  echo "BCACHE_K01=0"
  qemu-system-riscv64 --version | head -n 1
} > results/week04/hai/r08-c1.log
script -q -a -c "make qemu CPUS=3" results/week04/hai/r08-c1.log
```

Run the same xv6 commands in the same order. The source SHA, CPUS, block
count, test order and workload must match C0.

## 4. Restore the tracked configuration

After exiting QEMU:

```sh
git checkout -- kernel/param.h
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
