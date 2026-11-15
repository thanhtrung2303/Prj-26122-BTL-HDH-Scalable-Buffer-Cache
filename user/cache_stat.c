#include "kernel/types.h"
#include "kernel/cache_stats.h"
#include "user/user.h"

static void
print_hit_ratio(const char *name, uint64 hits, uint64 lookups)
{
  if (lookups == 0 || hits > lookups) {
    printf("%s: N/A\n", name);
    return;
  }

  // Accumulate hits / lookups 100 times without overflowing hits * 100.
  uint64 remainder = 0;
  uint64 until_carry = lookups - hits;
  int percent = 0;
  for (int i = 0; i < 100; i++) {
    if (remainder >= until_carry) {
      remainder -= until_carry;
      percent++;
    } else {
      remainder += hits;
    }
  }
  printf("%s: %d%%\n", name, percent);
}

int
main(int argc, char *argv[])
{
  struct cache_stats stats;

  if (argc == 2 && strcmp(argv[1], "reset") == 0) {
    if (cachereset() < 0) {
      fprintf(2, "cache_stat: reset failed\n");
      exit(1);
    }
    exit(0);
  }
  if (argc != 1) {
    fprintf(2, "usage: cache_stat [reset]\n");
    exit(1);
  }
  if (cachestat(&stats) < 0) {
    fprintf(2, "cache_stat: snapshot failed\n");
    exit(1);
  }

  printf("demand_lookups: %lu\n", stats.demand_lookups);
  printf("demand_lookup_hits: %lu\n", stats.demand_lookup_hits);
  printf("demand_lookup_misses: %lu\n", stats.demand_lookup_misses);
  printf("demand_disk_reads: %lu\n", stats.demand_disk_reads);

  printf("read_ahead_lookups: %lu\n", stats.read_ahead_lookups);
  printf("read_ahead_lookup_hits: %lu\n", stats.read_ahead_lookup_hits);
  printf("read_ahead_lookup_misses: %lu\n", stats.read_ahead_lookup_misses);
  printf("read_ahead_skips: %lu\n", stats.read_ahead_skips);
  printf("read_ahead_disk_reads: %lu\n", stats.read_ahead_disk_reads);

  printf("total_lookups: %lu\n",
         stats.demand_lookups + stats.read_ahead_lookups);
  printf("total_disk_reads: %lu\n",
         stats.demand_disk_reads + stats.read_ahead_disk_reads);
  print_hit_ratio("demand_hit_ratio", stats.demand_lookup_hits,
                  stats.demand_lookups);
  print_hit_ratio("read_ahead_hit_ratio", stats.read_ahead_lookup_hits,
                  stats.read_ahead_lookups);
  exit(0);
}
