#include "monitor/monitor.h"
#include "monitor/watchpoint.h"
#include "cpu/helper.h"
#include "device/i8259.h"
#include <setjmp.h>

/* The assembly code of instructions executed is only output to the screen
 * when the number of instructions executed is less than this value.
 * This is useful when you use the `si' command.
 * You can modify this value as you want.
 */
#define MAX_INSTR_TO_PRINT 10

int nemu_state = STOP;

int exec(swaddr_t);

char assembly[80];
char asm_buf[128];

/* Used with exception handling. */
jmp_buf jbuf;

static void intr_push(uint32_t value) {
	current_sreg = R_SS;
	reg_l(R_ESP) -= 4;
	swaddr_write(reg_l(R_ESP), 4, value);
}

void raise_intr(uint8_t no) {
	/* WB的作业，可借鉴，请勿直接复制粘贴 */
	Assert((uint32_t)no * 8 + 7 <= cpu.idtr.limit,
		"interrupt vector %u exceeds IDTR limit", no);

	GateDescriptor gate;
	gate.raw.low = lnaddr_read(cpu.idtr.base + (uint32_t)no * 8, 4);
	gate.raw.high = lnaddr_read(cpu.idtr.base + (uint32_t)no * 8 + 4, 4);
	Assert(gate.type_attr & 0x80, "interrupt vector %u is not present", no);

	intr_push(cpu.eflags.val);
	intr_push(cpu.cs.selector);
	intr_push(cpu.eip);

	uint8_t gate_type = gate.type_attr & 0xf;
	if(gate_type == 0xe) cpu.eflags.IF = 0; /* interrupt gate */
	else Assert(gate_type == 0xf, "unsupported gate type 0x%x", gate_type);

	cpu.cs.selector = gate.selector;
	sreg_load(R_CS);
	cpu.eip = gate.offset_15_0 | ((uint32_t)gate.offset_31_16 << 16);
	longjmp(jbuf, 1);
}

void print_bin_instr(swaddr_t eip, int len) {
	int i;
	int l = sprintf(asm_buf, "%8x:   ", eip);
	for(i = 0; i < len; i ++) {
		l += sprintf(asm_buf + l, "%02x ", instr_fetch(eip + i, 1));
	}
	sprintf(asm_buf + l, "%*.s", 50 - (12 + 3 * len), "");
}

/* This function will be called when an `int3' instruction is being executed. */
void do_int3() {
	printf("\nHit breakpoint at eip = 0x%08x\n", cpu.eip);
	nemu_state = STOP;
}

/* Simulate how the CPU works. */
void cpu_exec(volatile uint32_t n) {
	if(nemu_state == END) {
		printf("Program execution has ended. To restart the program, exit NEMU and run again.\n");
		return;
	}
	nemu_state = RUNNING;

#ifdef DEBUG
	volatile uint32_t n_temp = n;
#endif

	setjmp(jbuf);

	for(; n > 0; n --) {
		swaddr_t eip_temp = cpu.eip;
#ifdef DEBUG
		if((n & 0xffff) == 0) {
			/* Output some dots while executing the program. */
			fputc('.', stderr);
		}
#endif

		/* Execute one instruction, including instruction fetch,
		 * instruction decode, and the actual execution. */
		int instr_len = exec(cpu.eip);

		cpu.eip += instr_len;

#ifdef DEBUG
		print_bin_instr(eip_temp, instr_len);
		strcat(asm_buf, assembly);
		Log_write("%s\n", asm_buf);
		if(n_temp < MAX_INSTR_TO_PRINT) {
			printf("%s\n", asm_buf);
		}
#endif

		/* WB的作业，可借鉴，请勿直接复制粘贴 */
		if(check_wp(eip_temp)) {
			nemu_state = STOP;
		}

#ifdef HAS_DEVICE
		extern void device_update();
		device_update();
#endif

		if(nemu_state != RUNNING) { return; }

#ifdef HAS_DEVICE
		if(cpu.INTR && cpu.eflags.IF) {
			uint8_t no = i8259_query_intr();
			i8259_ack_intr();
			raise_intr(no);
		}
#endif
	}

	if(nemu_state == RUNNING) { nemu_state = STOP; }
}
