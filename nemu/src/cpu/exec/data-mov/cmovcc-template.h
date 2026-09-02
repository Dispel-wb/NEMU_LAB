#include "cpu/exec/template-start.h"
#ifndef def
#define def
/* WB的作业，可借鉴，请勿直接复制粘贴 */
static inline bool check_cc_o(){
	return cpu.eflags.OF;
}
static inline bool check_cc_no(){
	return !cpu.eflags.OF;
}
static inline bool check_cc_b(){
	return cpu.eflags.CF;
}
static inline bool check_cc_ae(){
	return !cpu.eflags.CF;
}
static inline bool check_cc_e(){
	return cpu.eflags.ZF;
}
static inline bool check_cc_ne(){
	return !cpu.eflags.ZF;
}
static inline bool check_cc_be(){
	return cpu.eflags.CF || cpu.eflags.ZF;
}
static inline bool check_cc_a(){
	return !cpu.eflags.CF && !cpu.eflags.ZF;
}
static inline bool check_cc_s(){
	return cpu.eflags.SF;
}
static inline bool check_cc_ns(){
	return !cpu.eflags.SF;
}
static inline bool check_cc_p(){
	return cpu.eflags.PF;
}
static inline bool check_cc_np(){
	return !cpu.eflags.PF;
}
static inline bool check_cc_l(){
	return cpu.eflags.SF != cpu.eflags.OF;
}
static inline bool check_cc_ge(){
	return cpu.eflags.SF == cpu.eflags.OF;
}
static inline bool check_cc_le(){
	return (cpu.eflags.SF != cpu.eflags.OF) || cpu.eflags.ZF;
}
static inline bool check_cc_g(){
	return !cpu.eflags.ZF && (cpu.eflags.SF == cpu.eflags.OF);
}
#endif
#define make_cmovcc_helper(cc) \
	make_helper(concat4(cmov, cc, _, SUFFIX)) { \
		int len = concat(decode_rm2r_, SUFFIX)(eip + 1); \
		(concat(check_cc_, cc)() ? OPERAND_W(op_dest, op_src->val) : 0 ); \
		print_asm(str(concat(cmov, cc)) str(SUFFIX) " %s,%s", op_src->str, op_dest->str); \
		return len + 1; \
	}

// make_helper(concat(cmove_, SUFFIX)){
// 	int len = concat(decode_rm2r_, SUFFIX)(eip + 1);
// 	if (cpu.ZF == 1) OPERAND_W(op_dest, op_src->val);
// 	print_asm_template2();
// 	return len+1;
// }
// make_helper(concat(cmovle_,SUFFIX)){
// 	int len = concat(decode_rm2r_, SUFFIX)(eip + 1);
// 	if ((cpu.SF ^ cpu.OF) | cpu.ZF) OPERAND_W(op_dest, op_src->val);
// 	print_asm_template2();
// 	return len+1;
// }
make_cmovcc_helper(o)
make_cmovcc_helper(no)
make_cmovcc_helper(b)
make_cmovcc_helper(ae)
make_cmovcc_helper(e)
make_cmovcc_helper(ne)
make_cmovcc_helper(be)
make_cmovcc_helper(a)
make_cmovcc_helper(s)
make_cmovcc_helper(ns)
make_cmovcc_helper(p)
make_cmovcc_helper(np)
make_cmovcc_helper(l)
make_cmovcc_helper(ge)
make_cmovcc_helper(le)
make_cmovcc_helper(g)

#include "cpu/exec/template-end.h"
