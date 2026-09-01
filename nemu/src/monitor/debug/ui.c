#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint32_t);
bool look_up_function(swaddr_t, const char **, swaddr_t *);

/* We use the `readline' library to provide more flexibility to read from stdin. */
char* rl_gets() {
	static char *line_read = NULL;

	if (line_read) {
		free(line_read);
		line_read = NULL;
	}

	line_read = readline("(nemu) ");

	if (line_read && *line_read) {
		add_history(line_read);
	}

	return line_read;
}

static int cmd_c(char *args) {
	(void)args;
	cpu_exec(-1);
	return 0;
}

static int cmd_q(char *args) {
	(void)args;
	return -1;
}

static int cmd_help(char *args);

static int cmd_si(char *args) {
	uint32_t count = 1;
	char *end = NULL;

	if(args != NULL) {
		unsigned long parsed = strtoul(args, &end, 10);
		if(end == args || *end != '\0' || parsed == 0 || parsed > UINT32_MAX) {
			printf("Usage: si [positive-count]\n");
			return 0;
		}
		count = parsed;
	}
	cpu_exec(count);
	return 0;
}

static void print_registers(void) {
	int i;
	for(i = 0; i < 8; i ++) {
		printf("%-3s 0x%08x  %-3s 0x%04x\n",
				regsl[i], reg_l(i), regsw[i], reg_w(i));
	}
	printf("eip 0x%08x  eflags 0x%08x\n", cpu.eip, cpu.eflags.val);
}

static int cmd_info(char *args) {
	if(args == NULL) {
		printf("Usage: info r | info w\n");
	}
	else if(strcmp(args, "r") == 0) {
		print_registers();
	}
	else if(strcmp(args, "w") == 0) {
		print_wp();
	}
	else {
		printf("Unknown info target '%s'. Use 'r' or 'w'.\n", args);
	}
	return 0;
}

static int cmd_x(char *args) {
	char *expression;
	char *end;
	unsigned long count;
	bool success;
	uint32_t address;
	unsigned long i;

	if(args == NULL) {
		printf("Usage: x N EXPR\n");
		return 0;
	}
	count = strtoul(args, &end, 10);
	if(end == args || count == 0) {
		printf("Usage: x N EXPR\n");
		return 0;
	}
	while(*end == ' ') end ++;
	expression = end;
	if(*expression == '\0') {
		printf("Usage: x N EXPR\n");
		return 0;
	}
	address = expr(expression, &success);
	if(!success) return 0;
	for(i = 0; i < count; i ++) {
		uint32_t current = address + i * 4;
		printf("0x%08x: 0x%08x\n", current, swaddr_read(current, 4));
	}
	return 0;
}

static int cmd_p(char *args) {
	bool success;
	uint32_t value;

	if(args == NULL) {
		printf("Usage: p EXPR\n");
		return 0;
	}
	value = expr(args, &success);
	if(success) {
		printf("%u (0x%08x)\n", value, value);
	}
	return 0;
}

static int cmd_w(char *args) {
	bool success;
	uint32_t value;
	WP *wp;

	if(args == NULL) {
		printf("Usage: w EXPR\n");
		return 0;
	}
	value = expr(args, &success);
	if(!success) return 0;
	/* WB的作业，可借鉴，请勿直接复制粘贴 */
	wp = new_wp(args, value);
	if(wp == NULL) {
		printf("Unable to create watchpoint (pool full or expression too long).\n");
	}
	else {
		printf("Watchpoint %d: %s (initial value 0x%08x)\n",
				wp->NO, wp->expression, wp->value);
	}
	return 0;
}

static int cmd_d(char *args) {
	char *end;
	long number;

	if(args == NULL) {
		printf("Usage: d N\n");
		return 0;
	}
	number = strtol(args, &end, 10);
	if(end == args || *end != '\0' || number < 0 || !free_wp(number)) {
		printf("No such watchpoint: %s\n", args);
	}
	else {
		printf("Watchpoint %ld deleted.\n", number);
	}
	return 0;
}

static int cmd_bt(char *args) {
	swaddr_t frame = cpu.ebp;
	swaddr_t pc = cpu.eip;
	int depth = 0;
	(void)args;

	while(pc != 0 && depth < 64) {
		const char *name = "??";
		swaddr_t start = 0;
		uint32_t argv[4] = {0};
		int i;
		look_up_function(pc, &name, &start);
		printf("#%d  0x%08x in %s+0x%x (", depth, pc, name, pc - start);
		if(frame != 0) {
			for(i = 0; i < 4; i ++) {
				argv[i] = swaddr_read(frame + 8 + i * 4, 4);
				printf("%s0x%x", i == 0 ? "" : ", ", argv[i]);
			}
		}
		printf(")\n");
		if(frame == 0) break;
		pc = swaddr_read(frame + 4, 4);
		frame = swaddr_read(frame, 4);
		depth ++;
	}
	return 0;
}

static struct {
	char *name;
	char *description;
	int (*handler) (char *);
} cmd_table [] = {
	{ "help", "Display informations about all supported commands", cmd_help },
	{ "c", "Continue the execution of the program", cmd_c },
	{ "q", "Exit NEMU", cmd_q },
	{ "si", "Execute one or N instructions", cmd_si },
	{ "info", "Display registers (r) or watchpoints (w)", cmd_info },
	{ "x", "Examine memory: x N EXPR", cmd_x },
	{ "p", "Evaluate an expression", cmd_p },
	{ "w", "Create a watchpoint", cmd_w },
	{ "d", "Delete a watchpoint", cmd_d },
	{ "bt", "Display the stack-frame chain", cmd_bt },
};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
	int i;

	if(args == NULL) {
		/* no argument given */
		for(i = 0; i < NR_CMD; i ++) {
			printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
		}
	}
	else {
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(args, cmd_table[i].name) == 0) {
				printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
				return 0;
			}
		}
		printf("Unknown command '%s'\n", args);
	}
	return 0;
}

void ui_mainloop() {
	while(1) {
		char *str = rl_gets();
		if(str == NULL) { return; }
		char *str_end = str + strlen(str);

		/* extract the first token as the command */
		char *cmd = strtok(str, " ");
		if(cmd == NULL) { continue; }

		/* treat the remaining string as the arguments,
		 * which may need further parsing
		 */
		char *args = cmd + strlen(cmd) + 1;
		if(args >= str_end) {
			args = NULL;
		}

#ifdef HAS_DEVICE
		extern void sdl_clear_event_queue(void);
		sdl_clear_event_queue();
#endif

		int i;
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(cmd, cmd_table[i].name) == 0) {
				if(cmd_table[i].handler(args) < 0) { return; }
				break;
			}
		}

		if(i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
	}
}
