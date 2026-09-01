#ifndef __REG_H__
#define __REG_H__

#include "common.h"
#include "../../../lib-common/x86-inc/cpu.h"

enum { R_EAX, R_ECX, R_EDX, R_EBX, R_ESP, R_EBP, R_ESI, R_EDI };
enum { R_AX, R_CX, R_DX, R_BX, R_SP, R_BP, R_SI, R_DI };
enum { R_AL, R_CL, R_DL, R_BL, R_AH, R_CH, R_DH, R_BH };
enum { R_ES, R_CS, R_SS, R_DS, R_FS, R_GS };

typedef struct {
	uint16_t selector;
	uint16_t attribute;
	uint32_t limit;
	uint32_t base;
} SegmentReg;

typedef union {
	struct {
		uint16_t limit_15_0;
		uint16_t base_15_0;
		uint32_t base_23_16 : 8;
		uint32_t type       : 4;
		uint32_t s          : 1;
		uint32_t dpl        : 2;
		uint32_t present    : 1;
		uint32_t limit_19_16: 4;
		uint32_t avl        : 1;
		uint32_t reserved   : 1;
		uint32_t db         : 1;
		uint32_t granularity: 1;
		uint32_t base_31_24 : 8;
	};
	struct { uint32_t low, high; } raw;
} SegmentDescriptor;

typedef union {
	struct {
		uint32_t present : 1;
		uint32_t rw      : 1;
		uint32_t user    : 1;
		uint32_t pwt     : 1;
		uint32_t pcd     : 1;
		uint32_t accessed: 1;
		uint32_t dirty   : 1;
		uint32_t pat     : 1;
		uint32_t global  : 1;
		uint32_t avail   : 3;
		uint32_t frame   : 20;
	};
	uint32_t val;
} PageEntry;

typedef struct {
	union {
		union {
			uint32_t _32;
			uint16_t _16;
			uint8_t _8[2];
		} gpr[8];
		struct { uint32_t eax, ecx, edx, ebx, esp, ebp, esi, edi; };
	};

	/* WB的作业，可借鉴，请勿直接复制粘贴 */
	swaddr_t eip;
	union {
		struct {
			uint32_t CF:1, pad0:1, PF:1, pad1:1, AF:1, pad2:1;
			uint32_t ZF:1, SF:1, TF:1, IF:1, DF:1, OF:1;
			uint32_t IOPL:2, NT:1, pad3:1;
			uint16_t pad4;
		};
		uint32_t val;
	} eflags;

	struct {
		uint32_t base;
		uint16_t limit;
	} gdtr;
	CR0 cr0;
	union {
		SegmentReg sreg[6];
		struct { SegmentReg es, cs, ss, ds, fs, gs; };
	};
	CR3 cr3;
} CPU_state;

extern CPU_state cpu;
extern uint8_t current_sreg;
void sreg_load(uint8_t sreg_num);

static inline int check_reg_index(int index) {
	assert(index >= 0 && index < 8);
	return index;
}

#define reg_l(index) (cpu.gpr[check_reg_index(index)]._32)
#define reg_w(index) (cpu.gpr[check_reg_index(index)]._16)
#define reg_b(index) (cpu.gpr[check_reg_index(index) & 0x3]._8[index >> 2])

extern const char* regsl[];
extern const char* regsw[];
extern const char* regsb[];

#endif
