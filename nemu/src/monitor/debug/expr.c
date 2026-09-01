#include "common.h"
#include "cpu/reg.h"
#include "memory/memory.h"
#include "monitor/expr.h"
#include <regex.h>
#include <stdlib.h>

enum {
	TK_NOTYPE = 256,
	TK_DECIMAL,
	TK_HEX,
	TK_REGISTER,
	TK_IDENTIFIER,
	TK_EQ,
	TK_NEQ,
	TK_AND,
	TK_OR,
	TK_NEG,
	TK_DEREF
};

static struct rule {
	const char *regex;
	int type;
} rules[] = {
	{" +", TK_NOTYPE},
	{"0[xX][0-9a-fA-F]+", TK_HEX},
	{"[0-9]+", TK_DECIMAL},
	{"\\$[a-zA-Z][a-zA-Z0-9]*", TK_REGISTER},
	{"[a-zA-Z_][a-zA-Z0-9_]*", TK_IDENTIFIER},
	{"==", TK_EQ},
	{"!=", TK_NEQ},
	{"&&", TK_AND},
	{"\\|\\|", TK_OR},
	{"\\+", '+'},
	{"-", '-'},
	{"\\*", '*'},
	{"/", '/'},
	{"!", '!'},
	{"\\(", '('},
	{"\\)", ')'}
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]))
#define MAX_TOKENS 256
#define MAX_TOKEN_TEXT 64

static regex_t re[NR_REGEX];

typedef struct {
	int type;
	char text[MAX_TOKEN_TEXT];
} Token;

static Token tokens[MAX_TOKENS];
static int nr_token;

void init_regex(void) {
	unsigned i;
	char error_msg[128];

	for(i = 0; i < NR_REGEX; i ++) {
		int ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
		if(ret != 0) {
			regerror(ret, &re[i], error_msg, sizeof(error_msg));
			Assert(ret == 0, "regex compilation failed: %s\n%s",
					error_msg, rules[i].regex);
		}
	}
}

static bool token_ends_value(int type) {
	return type == TK_DECIMAL || type == TK_HEX || type == TK_REGISTER ||
		type == TK_IDENTIFIER || type == ')';
}

/* WB的作业，可借鉴，请勿直接复制粘贴 */
static bool make_token(char *expression) {
	int position = 0;
	int length = strlen(expression);

	nr_token = 0;
	while(position < length) {
		unsigned i;
		regmatch_t match;
		for(i = 0; i < NR_REGEX; i ++) {
			if(regexec(&re[i], expression + position, 1, &match, 0) == 0 &&
					match.rm_so == 0) {
				int token_length = match.rm_eo;
				int type = rules[i].type;
				position += token_length;
				if(type == TK_NOTYPE) {
					break;
				}
				if(nr_token >= MAX_TOKENS || token_length >= MAX_TOKEN_TEXT) {
					printf("Expression is too long.\n");
					return false;
				}
				tokens[nr_token].type = type;
				memcpy(tokens[nr_token].text,
						expression + position - token_length, token_length);
				tokens[nr_token].text[token_length] = '\0';
				nr_token ++;
				break;
			}
		}
		if(i == NR_REGEX) {
			printf("Invalid token at position %d:\n%s\n%*s^\n",
					position, expression, position + 1, "");
			return false;
		}
	}

	for(position = 0; position < nr_token; position ++) {
		if((tokens[position].type == '-' || tokens[position].type == '*') &&
				(position == 0 || !token_ends_value(tokens[position - 1].type))) {
			tokens[position].type = tokens[position].type == '-' ? TK_NEG : TK_DEREF;
		}
	}
	return nr_token > 0;
}

static int precedence(int type) {
	switch(type) {
		case TK_OR: return 1;
		case TK_AND: return 2;
		case TK_EQ:
		case TK_NEQ: return 3;
		case '+':
		case '-': return 4;
		case '*':
		case '/': return 5;
		case '!':
		case TK_NEG:
		case TK_DEREF: return 6;
		default: return 0;
	}
}

static bool is_unary(int type) {
	return type == '!' || type == TK_NEG || type == TK_DEREF;
}

static bool enclosed_by_parentheses(int left, int right) {
	int depth = 0;
	int i;

	if(tokens[left].type != '(' || tokens[right].type != ')') {
		return false;
	}
	for(i = left; i <= right; i ++) {
		if(tokens[i].type == '(') depth ++;
		if(tokens[i].type == ')') depth --;
		if(depth == 0 && i < right) return false;
		if(depth < 0) return false;
	}
	return depth == 0;
}

static int dominant_operator(int left, int right, bool *success) {
	int depth = 0;
	int best_precedence = 100;
	int best = -1;
	int i;

	for(i = left; i <= right; i ++) {
		int type = tokens[i].type;
		if(type == '(') {
			depth ++;
			continue;
		}
		if(type == ')') {
			depth --;
			if(depth < 0) *success = false;
			continue;
		}
		if(depth == 0) {
			int current = precedence(type);
			if(current != 0 && (current < best_precedence ||
					(current == best_precedence && !is_unary(type)))) {
				best_precedence = current;
				best = i;
			}
		}
	}
	if(depth != 0) *success = false;
	return best;
}

static bool read_register(const char *name, uint32_t *value) {
	int i;

	for(i = 0; i < 8; i ++) {
		if(strcmp(name, regsl[i]) == 0) {
			*value = reg_l(i);
			return true;
		}
		if(strcmp(name, regsw[i]) == 0) {
			*value = reg_w(i);
			return true;
		}
		if(strcmp(name, regsb[i]) == 0) {
			*value = reg_b(i);
			return true;
		}
	}
	if(strcmp(name, "eip") == 0) {
		*value = cpu.eip;
		return true;
	}
	return false;
}

static uint32_t eval(int left, int right, bool *success) {
	int op;
	uint32_t lhs;
	uint32_t rhs;

	if(!*success || left > right) {
		*success = false;
		return 0;
	}
	while(left < right && enclosed_by_parentheses(left, right)) {
		left ++;
		right --;
	}
	if(left == right) {
		switch(tokens[left].type) {
			case TK_DECIMAL:
				return strtoul(tokens[left].text, NULL, 10);
			case TK_HEX:
				return strtoul(tokens[left].text, NULL, 16);
			case TK_REGISTER:
				if(read_register(tokens[left].text + 1, &lhs)) return lhs;
				printf("Unknown register: %s\n", tokens[left].text);
				break;
			case TK_IDENTIFIER:
				/* WB的作业，可借鉴，请勿直接复制粘贴 */
				if(look_up_symbol(tokens[left].text, &lhs)) {
					current_sreg = R_DS;
					return swaddr_read(lhs, 4);
				}
				printf("Unknown variable: %s\n", tokens[left].text);
				break;
			default:
				printf("Expected a value near '%s'.\n", tokens[left].text);
		}
		*success = false;
		return 0;
	}

	op = dominant_operator(left, right, success);
	if(!*success || op < 0) {
		printf("Malformed expression.\n");
		*success = false;
		return 0;
	}

	if(is_unary(tokens[op].type)) {
		if(op != left) {
			*success = false;
			return 0;
		}
		rhs = eval(op + 1, right, success);
		if(!*success) return 0;
		switch(tokens[op].type) {
			case '!': return !rhs;
			case TK_NEG: return -rhs;
			case TK_DEREF:
				current_sreg = R_DS;
				return swaddr_read(rhs, 4);
		}
	}

	lhs = eval(left, op - 1, success);
	if(!*success) return 0;
	if(tokens[op].type == TK_OR && lhs) return 1;
	if(tokens[op].type == TK_AND && !lhs) return 0;
	rhs = eval(op + 1, right, success);
	if(!*success) return 0;

	switch(tokens[op].type) {
		case '+': return lhs + rhs;
		case '-': return lhs - rhs;
		case '*': return lhs * rhs;
		case '/':
			if(rhs == 0) {
				printf("Division by zero.\n");
				*success = false;
				return 0;
			}
			return lhs / rhs;
		case TK_EQ: return lhs == rhs;
		case TK_NEQ: return lhs != rhs;
		case TK_AND: return lhs && rhs;
		case TK_OR: return lhs || rhs;
	}

	*success = false;
	return 0;
}

uint32_t expr(char *expression, bool *success) {
	if(success == NULL) return 0;
	*success = make_token(expression);
	if(!*success) return 0;
	return eval(0, nr_token - 1, success);
}
