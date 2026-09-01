#include "cpu/exec/template-start.h"

#define instr inc

static void do_execute () {
	DATA_TYPE ret = op_src->val + 1;
	OPERAND_W(op_src, ret);

	/* TODO: Update EFLAGS. */
	cpu.eflags.ZF = !ret;
    cpu.eflags.SF = ret >> ((DATA_BYTE << 3) - 1);
    cpu.eflags.OF = (ret < op_src -> val);
    ret ^= ret >> 4;
    ret ^= ret >> 2;
    ret ^= ret >> 1;
    ret &= 1;
    cpu.eflags.PF = !ret;

	print_asm_template1();
}

make_instr_helper(rm)
#if DATA_BYTE == 2 || DATA_BYTE == 4
make_instr_helper(r)
#endif

#include "cpu/exec/template-end.h"
