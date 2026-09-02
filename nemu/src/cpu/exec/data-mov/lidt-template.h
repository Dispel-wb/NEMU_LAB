#include "cpu/exec/template-start.h"

#define instr lidt


static void do_execute() {
	cpu.idtr.limit = swaddr_read(op_src->addr, 2);
	uint32_t base_low = swaddr_read(op_src->addr + 2, 2);
	uint32_t base_high = swaddr_read(op_src->addr + 4, op_src->size == 2 ? 1 : 2);
	cpu.idtr.base = base_low | (base_high << 16);
	print_asm_template1();
}


make_instr_helper(rm);


#include "cpu/exec/template-end.h"
