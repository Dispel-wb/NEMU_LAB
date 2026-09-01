#include "cpu/exec/template-start.h"
#include "cpu/decode/modrm.h"
#include "memory/tlb.h"

#define instr mov

static void do_execute() {
	OPERAND_W(op_dest, op_src->val);
	print_asm_template2();
}

make_instr_helper(i2r)
make_instr_helper(i2rm)
make_instr_helper(r2rm)
make_instr_helper(rm2r)

make_helper(concat(mov_a2moffs_, SUFFIX)) {
	swaddr_t addr = instr_fetch(eip + 1, 4);
	current_sreg = R_DS;
	MEM_W(addr, REG(R_EAX));

	print_asm("mov" str(SUFFIX) " %%%s,0x%x", REG_NAME(R_EAX), addr);
	return 5;
}

make_helper(concat(mov_moffs2a_, SUFFIX)) {
	swaddr_t addr = instr_fetch(eip + 1, 4);
	current_sreg = R_DS;
	REG(R_EAX) = MEM_R(addr);

	print_asm("mov" str(SUFFIX) " 0x%x,%%%s", addr, REG_NAME(R_EAX));
	return 5;
}

#if DATA_BYTE == 4
make_helper(mov_cr2r) {
	ModR_M modrm;
	modrm.val = instr_fetch(eip + 1, 1);
	Assert(modrm.mod == 3, "MOV from CR requires a register operand");
	if(modrm.reg == 0) reg_l(modrm.R_M) = cpu.cr0.val;
	else if(modrm.reg == 3) reg_l(modrm.R_M) = cpu.cr3.val;
	else Assert(0, "control register CR%u is not implemented", modrm.reg);
	print_asm("mov %%cr%u,%%%s", modrm.reg, regsl[modrm.R_M]);
	return 2;
}

make_helper(mov_r2cr) {
	ModR_M modrm;
	modrm.val = instr_fetch(eip + 1, 1);
	Assert(modrm.mod == 3, "MOV to CR requires a register operand");
	if(modrm.reg == 0) cpu.cr0.val = reg_l(modrm.R_M);
	else if(modrm.reg == 3) {
		cpu.cr3.val = reg_l(modrm.R_M);
		init_tlb();
	}
	else Assert(0, "control register CR%u is not implemented", modrm.reg);
	print_asm("mov %%%s,%%cr%u", regsl[modrm.R_M], modrm.reg);
	return 2;
}
#endif

#if DATA_BYTE == 2
make_helper(mov_sreg2rm) {
	ModR_M modrm;
	modrm.val = instr_fetch(eip + 1, 1);
	Assert(modrm.reg < 6, "invalid segment register %u", modrm.reg);

	Operand rm, ignored;
	memset(&rm, 0, sizeof(rm));
	memset(&ignored, 0, sizeof(ignored));
	rm.size = ignored.size = 2;
	int len = read_ModR_M(eip + 1, &rm, &ignored);
	cpu.sreg[modrm.reg].selector = (uint16_t)rm.val;
	sreg_load(modrm.reg);
	print_asm("mov %s,%%sreg%u", rm.str, modrm.reg);
	return len + 1;
}
#endif

#include "cpu/exec/template-end.h"
