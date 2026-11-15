#ifndef XV6_CACHE_STATS_H
#define XV6_CACHE_STATS_H

enum cache_source { DEMAND, READ_AHEAD };

struct cache_stats {
  uint64 demand_lookups;
  uint64 demand_lookup_hits;
  uint64 demand_lookup_misses;
  uint64 demand_disk_reads;
  uint64 read_ahead_lookups;
  uint64 read_ahead_lookup_hits;
  uint64 read_ahead_lookup_misses;
  uint64 read_ahead_skips;
  uint64 read_ahead_disk_reads;
};

#endif
