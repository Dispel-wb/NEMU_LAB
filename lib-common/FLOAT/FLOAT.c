#include "FLOAT.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>

FLOAT F_mul_F(FLOAT a, FLOAT b) {
	int64_t product = (int64_t)a * b;
	return product >> 16;
}

FLOAT F_div_F(FLOAT a, FLOAT b) {
	/* Dividing two 64-bit integers needs the support of another library
	 * `libgcc', other than newlib. It is a dirty work to port `libgcc'
	 * to NEMU. In fact, it is unnecessary to perform a "64/64" division
	 * here. A "64/32" division is enough.
	 *
	 * To perform a "64/32" division, you can use the x86 instruction
	 * `div' or `idiv' by inline assembly. We provide a template for you
	 * to prevent you from uncessary details.
	 *
	 *     asm volatile ("??? %2" : "=a"(???), "=d"(???) : "r"(???), "a"(???), "d"(???));
	 *
	 * If you want to use the template above, you should fill the "???"
	 * correctly. For more information, please read the i386 manual for
	 * division instructions, and search the Internet about "inline assembly".
	 * It is OK not to use the template above, but you should figure
	 * out another way to perform the division.
	 */

	nemu_assert(b != 0);
	bool negative = (a < 0) != (b < 0);
	uint32_t dividend = a < 0 ? -(int64_t)a : a;
	uint32_t divisor = b < 0 ? -(int64_t)b : b;
	uint32_t quotient = dividend / divisor;
	uint32_t remainder = dividend % divisor;
	int i;

	/* WB的作业，可借鉴，请勿直接复制粘贴 */
	quotient <<= 16;
	for(i = 0; i < 16; i ++) {
		remainder <<= 1;
		if(remainder >= divisor) {
			remainder -= divisor;
			quotient |= 1u << (15 - i);
		}
	}
	return negative ? -(FLOAT)quotient : (FLOAT)quotient;
}

FLOAT f2F(float a) {
	/* You should figure out how to convert `a' into FLOAT without
	 * introducing x87 floating point instructions. Else you can
	 * not run this code in NEMU before implementing x87 floating
	 * point instructions, which is contrary to our expectation.
	 *
	 * Hint: The bit representation of `a' is already on the
	 * stack. How do you retrieve it to another variable without
	 * performing arithmetic operations on it directly?
	 */

	uint32_t bits;
	uint32_t fraction;
	int exponent;
	bool negative;

	memcpy(&bits, &a, sizeof(bits));
	negative = bits >> 31;
	exponent = (bits >> 23) & 0xff;
	fraction = bits & 0x7fffff;
	if(exponent == 0) return 0;
	if(exponent == 0xff) return negative ? INT32_MIN : INT32_MAX;

	fraction |= 1u << 23;
	exponent -= 134;
	if(exponent >= 0) {
		if(exponent > 7 || fraction > ((uint32_t)INT32_MAX >> exponent)) {
			return negative ? INT32_MIN : INT32_MAX;
		}
		fraction <<= exponent;
	}
	else {
		if(exponent <= -31) return 0;
		fraction >>= -exponent;
	}
	return negative ? -(FLOAT)fraction : (FLOAT)fraction;
}

FLOAT Fabs(FLOAT a) {
	if(a == INT32_MIN) return INT32_MAX;
	return a < 0 ? -a : a;
}

/* Functions below are already implemented */

FLOAT sqrt(FLOAT x) {
	FLOAT dt, t = int2F(2);

	do {
		dt = F_div_int((F_div_F(x, t) - t), 2);
		t += dt;
	} while(Fabs(dt) > f2F(1e-4));

	return t;
}

FLOAT pow(FLOAT x, FLOAT y) {
	/* we only compute x^0.333 */
	FLOAT t2, dt, t = int2F(2);

	do {
		t2 = F_mul_F(t, t);
		dt = (F_div_F(x, t2) - t) / 3;
		t += dt;
	} while(Fabs(dt) > f2F(1e-4));

	return t;
}
