#ifndef CONFIG_H
#define CONFIG_H

#define CONFIG config
#define CACHE cache
#define ASSOC_ E
#define ASSOC CONFIG.ASSOC_
#define SET_BITS_ s
#define SET_BITS CONFIG.SET_BITS_
#define CACHE_SIZE (1 << SET_BITS)
#define BLOCK_BITS_ b
#define BLOCK_BITS CONFIG.BLOCK_BITS_
#define BLOCK_SIZE (1 << BLOCK_BITS)
#define VERBOSE_ verbose
#define VERBOSE CONFIG.VERBOSE_
#define TRACE_ tracefile
#define TRACE CONFIG.TRACE_
#define HELP_ help
#define HELP CONFIG.HELP_
#define HITS_ hits
#define HITS CONFIG.HITS_
#define MISSES_ misses
#define MISSES CONFIG.MISSES_
#define EVICTIONS_ evictions
#define EVICTIONS CONFIG.EVICTIONS_

#endif
