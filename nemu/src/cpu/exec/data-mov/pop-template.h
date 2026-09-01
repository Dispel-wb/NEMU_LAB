#include "cpu/exec/template-start.h"

#define instr pop


static void do_execute() {
	current_sreg = R_SS;
	DATA_TYPE value = MEM_R(reg_l(R_ESP));
	reg_l(R_ESP) += DATA_BYTE;
	if(op_src->type == OP_TYPE_MEM) current_sreg = op_src->sreg;
	OPERAND_W(op_src, value);

    print_asm_template1();
}


make_instr_helper(r)
make_instr_helper(rm)


#include "cpu/exec/template-end.h"
