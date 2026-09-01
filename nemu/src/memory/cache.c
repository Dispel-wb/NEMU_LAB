#include "common.h"
#include "memory/cache.h"
#include "burst.h"

void ddr3_read_replace(hwaddr_t addr, void *data);
void ddr3_write_replace(hwaddr_t addr, void *data, uint8_t *mask);

CacheL1Line cache1[CACHE_L1_LINE_COUNT];
CacheL2Line cache2[CACHE_L2_LINE_COUNT];
uint64_t cache_cycles;

static uint32_t replacement_state;

static uint32_t replacement_way(uint32_t ways) {
	/* WB的作业，可借鉴，请勿直接复制粘贴 */
	replacement_state ^= replacement_state << 13;
	replacement_state ^= replacement_state >> 17;
	replacement_state ^= replacement_state << 5;
	return replacement_state % ways;
}

void init_cache(void) {
	memset(cache1, 0, sizeof(cache1));
	memset(cache2, 0, sizeof(cache2));
	cache_cycles = 0;
	replacement_state = 0x30252471u;
}

int read_cache2(hwaddr_t addr) {
	uint32_t set = (addr >> 6) & (CACHE_L2_SET_COUNT - 1);
	uint32_t tag = addr >> 18;
	uint32_t first = set * CACHE_L2_WAY_COUNT;
	uint32_t way;

	for(way = 0; way < CACHE_L2_WAY_COUNT; ++way) {
		CacheL2Line *line = &cache2[first + way];
		if(line->valid && line->tag == tag) {
			cache_cycles += 10;
			return first + way;
		}
	}

	cache_cycles += 200;
	for(way = 0; way < CACHE_L2_WAY_COUNT; ++way) {
		if(!cache2[first + way].valid) break;
	}
	if(way == CACHE_L2_WAY_COUNT) way = replacement_way(CACHE_L2_WAY_COUNT);

	CacheL2Line *victim = &cache2[first + way];
	if(victim->valid && victim->dirty) {
		hwaddr_t old_block = (victim->tag << 18) | (set << 6);
		uint8_t mask[BURST_LEN];
		memset(mask, 1, sizeof(mask));
		uint32_t off;
		for(off = 0; off < CACHE_L2_BLOCK_SIZE; off += BURST_LEN) {
			ddr3_write_replace(old_block + off, victim->data + off, mask);
		}
	}

	hwaddr_t block = addr & ~(CACHE_L2_BLOCK_SIZE - 1);
	uint32_t off;
	for(off = 0; off < CACHE_L2_BLOCK_SIZE; off += BURST_LEN) {
		ddr3_read_replace(block + off, victim->data + off);
	}
	victim->valid = true;
	victim->dirty = false;
	victim->tag = tag;
	return first + way;
}

int read_cache1(hwaddr_t addr) {
	uint32_t set = (addr >> 6) & (CACHE_L1_SET_COUNT - 1);
	uint32_t tag = addr >> 13;
	uint32_t first = set * CACHE_L1_WAY_COUNT;
	uint32_t way;

	for(way = 0; way < CACHE_L1_WAY_COUNT; ++way) {
		CacheL1Line *line = &cache1[first + way];
		if(line->valid && line->tag == tag) {
			cache_cycles += 2;
			return first + way;
		}
	}

	for(way = 0; way < CACHE_L1_WAY_COUNT; ++way) {
		if(!cache1[first + way].valid) break;
	}
	if(way == CACHE_L1_WAY_COUNT) way = replacement_way(CACHE_L1_WAY_COUNT);

	int source = read_cache2(addr);
	CacheL1Line *victim = &cache1[first + way];
	memcpy(victim->data, cache2[source].data, CACHE_L1_BLOCK_SIZE);
	victim->tag = tag;
	victim->valid = true;
	return first + way;
}

void write_cache2(hwaddr_t addr, size_t len, uint32_t data) {
	uint32_t offset = addr & (CACHE_L2_BLOCK_SIZE - 1);
	if(offset + len > CACHE_L2_BLOCK_SIZE) {
		size_t first_len = CACHE_L2_BLOCK_SIZE - offset;
		write_cache2(addr, first_len, data);
		write_cache2(addr + first_len, len - first_len, data >> (first_len * 8));
		return;
	}

	int index = read_cache2(addr);       /* write allocate */
	memcpy(cache2[index].data + offset, &data, len);
	cache2[index].dirty = true;          /* write back */
}

void write_cache1(hwaddr_t addr, size_t len, uint32_t data) {
	uint32_t offset = addr & (CACHE_L1_BLOCK_SIZE - 1);
	if(offset + len > CACHE_L1_BLOCK_SIZE) {
		size_t first_len = CACHE_L1_BLOCK_SIZE - offset;
		write_cache1(addr, first_len, data);
		write_cache1(addr + first_len, len - first_len, data >> (first_len * 8));
		return;
	}

	uint32_t set = (addr >> 6) & (CACHE_L1_SET_COUNT - 1);
	uint32_t tag = addr >> 13;
	uint32_t first = set * CACHE_L1_WAY_COUNT;
	uint32_t way;
	for(way = 0; way < CACHE_L1_WAY_COUNT; ++way) {
		CacheL1Line *line = &cache1[first + way];
		if(line->valid && line->tag == tag) {
			memcpy(line->data + offset, &data, len);
			break;
		}
	}

	/* L1 is write-through and not-write-allocate; L2 is the next level. */
	write_cache2(addr, len, data);
}
