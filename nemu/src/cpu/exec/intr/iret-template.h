#include "cpu/exec/template-start.h"

#define instr iret

static DATA_TYPE concat(iret_pop_, SUFFIX)(void) {
	current_sreg = R_SS;
	DATA_TYPE value = swaddr_read(reg_l(R_ESP), DATA_BYTE);
	reg_l(R_ESP) += DATA_BYTE;
	return value;
}

make_helper(concat(iret_, SUFFIX)) {
	cpu.eip = concat(iret_pop_, SUFFIX)();
	cpu.cs.selector = concat(iret_pop_, SUFFIX)();
	cpu.eflags.val = concat(iret_pop_, SUFFIX)();
	if(cpu.cr0.protect_enable) sreg_load(R_CS);
	print_asm("iret");
	return 0;
}

#include "cpu/exec/template-end.h"
