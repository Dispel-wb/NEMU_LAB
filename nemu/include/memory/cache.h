#ifndef __CACHE_H__
#define __CACHE_H__

#include "common.h"

#define CACHE_L1_BLOCK_SIZE 64
#define CACHE_L1_SET_COUNT 128
#define CACHE_L1_WAY_COUNT 8
#define CACHE_L1_LINE_COUNT (CACHE_L1_SET_COUNT * CACHE_L1_WAY_COUNT)

#define CACHE_L2_BLOCK_SIZE 64
#define CACHE_L2_SET_COUNT 4096
#define CACHE_L2_WAY_COUNT 16
#define CACHE_L2_LINE_COUNT (CACHE_L2_SET_COUNT * CACHE_L2_WAY_COUNT)

/* Compatibility names used by the memory interface and PA3 tests. */
#define Cache_L1_Block_Size CACHE_L1_BLOCK_SIZE

typedef struct {
	uint8_t data[CACHE_L1_BLOCK_SIZE];
	uint32_t tag;
	bool valid;
} CacheL1Line;

typedef struct {
	uint8_t data[CACHE_L2_BLOCK_SIZE];
	uint32_t tag;
	bool valid;
	bool dirty;
} CacheL2Line;

extern CacheL1Line cache1[CACHE_L1_LINE_COUNT];
extern CacheL2Line cache2[CACHE_L2_LINE_COUNT];
extern uint64_t cache_cycles;

void init_cache(void);
int read_cache1(hwaddr_t addr);
void write_cache1(hwaddr_t addr, size_t len, uint32_t data);
int read_cache2(hwaddr_t addr);
void write_cache2(hwaddr_t addr, size_t len, uint32_t data);

#endif
