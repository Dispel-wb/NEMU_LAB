#include "cpu/exec/template-start.h"

#define instr jmp

static void do_execute () {
	DATA_TYPE_S imm = op_src -> val;
    if (op_src -> type == OP_TYPE_IMM){
        cpu.eip += imm;
    }else {
        int len = concat(decode_rm_, SUFFIX)(cpu.eip + 1);
        cpu.eip = imm - len - 1;
    }
    print_asm_template1();
}

make_instr_helper(i)
make_instr_helper(rm)
#if DATA_BYTE == 4

make_helper(ljmp){
	uint32_t offset = instr_fetch(eip + 1, 4);
	uint16_t selector = instr_fetch(eip + 5, 2);
	cpu.cs.selector = selector;
	sreg_load(R_CS);
	cpu.eip = offset - 7;
	print_asm("ljmp $0x%x,$0x%x", selector, offset);
	return 7;
}
#endif
#include "cpu/exec/template-end.h"
