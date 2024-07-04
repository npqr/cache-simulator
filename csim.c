#include <assert.h>
#include <getopt.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "cachelab.h"
#include "config.h"

void parseArg(int argc, char *argv[]);
int init();
void simulate(FILE *trace);
uint64_t getTag(uint64_t addr);
uint64_t getSet(uint64_t addr);
uint16_t getOffset(uint64_t addr);
void checkCache(uint64_t tag, uint64_t set, uint16_t offset, int counter);

typedef struct CacheConfig {
    bool VERBOSE_;
    int SET_BITS_;
    int ASSOC_;
    int BLOCK_BITS_;
    FILE *TRACE_;
    int HITS_;
    int MISSES_;
    int EVICTIONS_;
} CacheConfig;

typedef struct CacheLine {
    bool valid;
    uint64_t tag;
    uint64_t lru;
} CacheLine;

typedef CacheLine *CacheSet;
typedef CacheSet *CacheTable;

CacheTable CACHE;
CacheConfig CONFIG;

int main(int argc, char *argv[]) {
    parseArg(argc, argv);
    init();
    simulate(TRACE);
    printSummary(HITS, MISSES, EVICTIONS);
    return 0;
}

void parseArg(int argc, char *argv[]) {
    if (argc == 1) {
        printf("Usage: %s [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n", argv[0]);
        exit(0);
    }

    int opt;
    while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
        switch (opt) {
            case 'h':
                printf("Usage: %s [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n", argv[0]);
                printf("Options:\n");
                printf("  -h         Print this help message.\n");
                printf("  -v         Optional verbose flag.\n");
                printf("  -s <s>     Number of set index bits.\n");
                printf("  -E <E>     Associativity.\n");
                printf("  -b <b>     Number of block bits.\n");
                printf("  -t <tracefile>   Name of the valgrind trace to replay.\n");
                printf("\nExamples:\n");
                printf("  linux>  %s -s 4 -E 1 -b 4 -t traces/yi.trace\n", argv[0]);
                printf("  linux>  %s -v -s 8 -E 2 -b 4 -t traces/yi.trace\n", argv[0]);
                exit(0);
            case 'v':
                VERBOSE = true;
                break;
            case 's':
                SET_BITS = atoi(optarg);
                break;
            case 'E':
                ASSOC = atoi(optarg);
                break;
            case 'b':
                BLOCK_BITS = atoi(optarg);
                break;
            case 't':
                TRACE = fopen(optarg, "r");
                break;
            default:
                printf("Usage: %s [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n", argv[0]);
                exit(0);
        }
    }
}

int init() {
    CACHE = malloc(CACHE_SIZE * sizeof(CacheSet));

    if (CACHE == NULL) {
        fprintf(stderr, "Failed to allocate memory for cache\n");
        exit(1);
    }

    for (int i = 0; i < CACHE_SIZE; i++) {
        CACHE[i] = malloc(ASSOC * sizeof(CacheLine));

        if (CACHE[i] == NULL) {
            fprintf(stderr, "Failed to allocate memory for cache set\n");
            exit(1);
        }

        for (int j = 0; j < ASSOC; j++) {
            CACHE[i][j].valid = false;
            CACHE[i][j].tag = 0;
            CACHE[i][j].lru = 0;
        }
    }

    return 0;
}

void simulate(FILE *trace) {
    char operation;
    uint64_t addr;
    int size;
    int counter = 0;

    if(TRACE == NULL) {
        fprintf(stderr, "Failed to open trace file\n");
        exit(1);
    }

    while (fscanf(TRACE, " %c %lx,%d", &operation, &addr, &size) != EOF) {
        ++counter;
        if (operation == 'I') {
            continue;
        }

        if (VERBOSE)
            printf("%c %lx,%d ", operation, addr, size);

        uint64_t tag = getTag(addr);
        uint64_t set = getSet(addr);
        uint16_t offset = getOffset(addr);

        switch (operation) {
            case 'L':
                checkCache(tag, set, offset, counter);
                break;
            case 'M':
                checkCache(tag, set, offset, counter);
                checkCache(tag, set, offset, counter);
                break;
            case 'S':
                checkCache(tag, set, offset, counter);
                break;
            default:
                break;
        }
    }

    // Free cache
    for (int i = 0; i < CACHE_SIZE; i++) {
        assert(CACHE[i] != NULL);
        free(CACHE[i]);
    }
    if (CACHE != NULL)
        free(CACHE);

    fclose(TRACE);
    return;
}

// Breakdown of address: [ tag | set | offset ] 

uint64_t getTag(uint64_t addr) {
    return addr >> (SET_BITS + BLOCK_BITS);
}

uint64_t getSet(uint64_t addr) {
    return (addr >> BLOCK_BITS) & ((1 << SET_BITS) - 1);
}

uint16_t getOffset(uint64_t addr) {
    return addr & ((1 << BLOCK_BITS) - 1);
}

void checkCache(uint64_t tag, uint64_t set, uint16_t offset, int counter) {
    for (int i = 0; i < ASSOC; i++) {
        if (CACHE[set][i].valid && CACHE[set][i].tag == tag) {
            CACHE[set][i].lru = counter;
            ++HITS;
            if (VERBOSE)
                printf("HIT\n");
            return;
        }
    }

    // Cache miss
    ++MISSES;
    if (VERBOSE)
        printf("MISS\n");

    int new_oldest = 0;
    for (int i = 0; i < ASSOC; i++) {
        if (!CACHE[set][i].valid) {
            CACHE[set][i].valid = true;
            CACHE[set][i].tag = tag;
            CACHE[set][i].lru = counter;
            return;
        }

        // Find the least recently used cache line
        if (CACHE[set][new_oldest].lru > CACHE[set][i].lru) {
            new_oldest = i;
        }
    }

    // Evict the least recently used cache line
    ++EVICTIONS;
    if (VERBOSE)
        printf("EVICT\n");

    CACHE[set][new_oldest].valid = true;
    CACHE[set][new_oldest].tag = tag;
    CACHE[set][new_oldest].lru = counter;
    return;
}
