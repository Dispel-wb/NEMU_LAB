#include "cpu/exec/helper.h"

make_helper(setcc_rm_b) {
	int len = decode_rm_b(eip + 1);
	bool value = false;

	/* WB的作业，可借鉴，请勿直接复制粘贴 */
	switch(ops_decoded.opcode & 0xf) {
		case 0x0: value = cpu.eflags.OF; break;
		case 0x1: value = !cpu.eflags.OF; break;
		case 0x2: value = cpu.eflags.CF; break;
		case 0x3: value = !cpu.eflags.CF; break;
		case 0x4: value = cpu.eflags.ZF; break;
		case 0x5: value = !cpu.eflags.ZF; break;
		case 0x6: value = cpu.eflags.CF || cpu.eflags.ZF; break;
		case 0x7: value = !cpu.eflags.CF && !cpu.eflags.ZF; break;
		case 0x8: value = cpu.eflags.SF; break;
		case 0x9: value = !cpu.eflags.SF; break;
		case 0xa: value = cpu.eflags.PF; break;
		case 0xb: value = !cpu.eflags.PF; break;
		case 0xc: value = cpu.eflags.SF != cpu.eflags.OF; break;
		case 0xd: value = cpu.eflags.SF == cpu.eflags.OF; break;
		case 0xe: value = cpu.eflags.ZF || (cpu.eflags.SF != cpu.eflags.OF); break;
		case 0xf: value = !cpu.eflags.ZF && (cpu.eflags.SF == cpu.eflags.OF); break;
	}
	write_operand_b(op_src, value);
	print_asm("setcc %s", op_src->str);
	return len + 1;
}
