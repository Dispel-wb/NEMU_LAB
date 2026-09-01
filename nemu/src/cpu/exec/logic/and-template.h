#include "cpu/exec/template-start.h"

#define instr and

static void do_execute () {
	DATA_TYPE ret = op_dest->val & op_src->val;
	OPERAND_W(op_dest, ret);

	/* TODO: Update EFLAGS. */
	cpu.eflags.CF = 0;
	cpu.eflags.OF = 0;
	cpu.eflags.ZF = !ret;
    cpu.eflags.SF = ret >> ((DATA_BYTE << 3) - 1);
	ret ^= ret >> 4;
    ret ^= ret >> 2;
    ret ^= ret >> 1;
    ret &= 1;
    cpu.eflags.PF = !ret;
	print_asm_template2();
}

make_instr_helper(i2a)
make_instr_helper(i2rm)
#if DATA_BYTE == 2 || DATA_BYTE == 4
make_instr_helper(si2rm)
#endif
make_instr_helper(r2rm)
make_instr_helper(rm2r)

#include "cpu/exec/template-end.h"
