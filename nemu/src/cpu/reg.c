#include "nemu.h"
#include <stdlib.h>
#include <time.h>

CPU_state cpu;
uint8_t current_sreg = R_DS;

const char *regsl[] = {"eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi"};
const char *regsw[] = {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"};
const char *regsb[] = {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"};

void reg_test() {
	srand(time(0));
	uint32_t sample[8];
	uint32_t eip_sample = rand();
	cpu.eip = eip_sample;

	int i;
	for(i = R_EAX; i <= R_EDI; i ++) {
		sample[i] = rand();
		reg_l(i) = sample[i];
		assert(reg_w(i) == (sample[i] & 0xffff));
	}

	assert(reg_b(R_AL) == (sample[R_EAX] & 0xff));
	assert(reg_b(R_AH) == ((sample[R_EAX] >> 8) & 0xff));
	assert(reg_b(R_BL) == (sample[R_EBX] & 0xff));
	assert(reg_b(R_BH) == ((sample[R_EBX] >> 8) & 0xff));
	assert(reg_b(R_CL) == (sample[R_ECX] & 0xff));
	assert(reg_b(R_CH) == ((sample[R_ECX] >> 8) & 0xff));
	assert(reg_b(R_DL) == (sample[R_EDX] & 0xff));
	assert(reg_b(R_DH) == ((sample[R_EDX] >> 8) & 0xff));

	assert(sample[R_EAX] == cpu.eax);
	assert(sample[R_ECX] == cpu.ecx);
	assert(sample[R_EDX] == cpu.edx);
	assert(sample[R_EBX] == cpu.ebx);
	assert(sample[R_ESP] == cpu.esp);
	assert(sample[R_EBP] == cpu.ebp);
	assert(sample[R_ESI] == cpu.esi);
	assert(sample[R_EDI] == cpu.edi);

	assert(eip_sample == cpu.eip);
}

void sreg_load(uint8_t sreg_num){
	Assert(sreg_num < 6, "invalid segment register %u", sreg_num);
	Assert(cpu.cr0.protect_enable, "segment load outside protected mode");

	uint16_t selector = cpu.sreg[sreg_num].selector;
	Assert((selector & 0x4) == 0, "LDT is not implemented");
	uint32_t offset = (selector >> 3) * 8;
	Assert(offset + 7 <= cpu.gdtr.limit, "segment selector exceeds GDTR limit");

	SegmentDescriptor desc;
	desc.raw.low = lnaddr_read(cpu.gdtr.base + offset, 4);
	desc.raw.high = lnaddr_read(cpu.gdtr.base + offset + 4, 4);
	Assert(desc.present, "segment is not present");

	uint32_t base = desc.base_15_0 |
		(desc.base_23_16 << 16) | (desc.base_31_24 << 24);
	uint32_t limit = desc.limit_15_0 | (desc.limit_19_16 << 16);
	if(desc.granularity) limit = (limit << 12) | 0xfff;

	cpu.sreg[sreg_num].base = base;
	cpu.sreg[sreg_num].limit = limit;
	cpu.sreg[sreg_num].attribute = (uint16_t)(desc.raw.high >> 8);
}
