#include "monitor/watchpoint.h"
#include "monitor/expr.h"

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;

void init_wp_pool(void) {
	int i;
	for(i = 0; i < NR_WP; i ++) {
		wp_pool[i].NO = i;
		wp_pool[i].next = &wp_pool[i + 1];
	}
	wp_pool[NR_WP - 1].next = NULL;

	head = NULL;
	free_ = wp_pool;
}

/* WB的作业，可借鉴，请勿直接复制粘贴 */
WP *new_wp(const char *expression, uint32_t value) {
	WP *wp;

	if(free_ == NULL || expression == NULL ||
			strlen(expression) >= sizeof(wp_pool[0].expression)) {
		return NULL;
	}

	wp = free_;
	free_ = free_->next;
	wp->next = head;
	wp->value = value;
	strcpy(wp->expression, expression);
	head = wp;
	return wp;
}

bool free_wp(int number) {
	WP **link = &head;

	while(*link != NULL && (*link)->NO != number) {
		link = &(*link)->next;
	}
	if(*link == NULL) {
		return false;
	}

	WP *wp = *link;
	*link = wp->next;
	wp->next = free_;
	free_ = wp;
	return true;
}

void print_wp(void) {
	WP *wp;

	if(head == NULL) {
		printf("No watchpoints.\n");
		return;
	}

	printf("Num\tValue\t\tExpression\n");
	for(wp = head; wp != NULL; wp = wp->next) {
		printf("%d\t0x%08x\t%s\n", wp->NO, wp->value, wp->expression);
	}
}

bool check_wp(swaddr_t eip) {
	bool changed = false;
	WP *wp;

	for(wp = head; wp != NULL; wp = wp->next) {
		bool success = true;
		uint32_t current = expr(wp->expression, &success);
		if(!success) {
			printf("Watchpoint %d expression is no longer valid: %s\n",
					wp->NO, wp->expression);
			continue;
		}
		if(current != wp->value) {
			printf("Hint watchpoint %d at address 0x%08x\n", wp->NO, eip);
			printf("Expression: %s\n", wp->expression);
			printf("Old value = 0x%08x\nNew value = 0x%08x\n",
					wp->value, current);
			wp->value = current;
			changed = true;
		}
	}
	return changed;
}
