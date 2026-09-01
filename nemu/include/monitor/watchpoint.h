#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

typedef struct watchpoint {
	int NO;
	struct watchpoint *next;
	uint32_t value;
	char expression[128];
} WP;

void init_wp_pool(void);
WP *new_wp(const char *expression, uint32_t value);
bool free_wp(int number);
void print_wp(void);
bool check_wp(swaddr_t eip);

#endif
