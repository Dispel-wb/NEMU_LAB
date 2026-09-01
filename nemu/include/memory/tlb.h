#ifndef __TLB_H__
#define __TLB_H__

#include "common.h"

#define TLB_SIZE 64

typedef struct {
	bool valid;
	uint32_t tag;
	uint32_t page_num;
} TLBEntry;

extern TLBEntry tlb[TLB_SIZE];

void init_tlb(void);
int read_tlb(uint32_t lnaddr);
void write_tlb(uint32_t lnaddr, uint32_t hwaddr);

#endif
