#include "common.h"
#include "memory/tlb.h"

TLBEntry tlb[TLB_SIZE];
static uint32_t tlb_replace;

void init_tlb(void) {
	memset(tlb, 0, sizeof(tlb));
	tlb_replace = 0;
}

int read_tlb(uint32_t lnaddr) {
	uint32_t vpn = lnaddr >> 12;
	int i;
	for(i = 0; i < TLB_SIZE; ++i) {
		if(tlb[i].valid && tlb[i].tag == vpn) return i;
	}
	return -1;
}

void write_tlb(uint32_t lnaddr, uint32_t hwaddr) {
	int i;
	for(i = 0; i < TLB_SIZE; ++i) {
		if(!tlb[i].valid) break;
	}
	if(i == TLB_SIZE) {
		tlb_replace = (tlb_replace * 1103515245u + 12345u) & 0x7fffffffu;
		i = tlb_replace % TLB_SIZE;
	}
	tlb[i].valid = true;
	tlb[i].tag = lnaddr >> 12;
	tlb[i].page_num = hwaddr >> 12;
}
