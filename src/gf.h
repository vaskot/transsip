/*
 * transsip - the telephony toolkit
 * By Daniel Borkmann <daniel@transsip.org>
 * Copyright 2012 Daniel Borkmann <dborkma@tik.ee.ethz.ch>
 * Subject to the GPL, version 2.
 * Based on Bhaskar Biswas and Nicolas Sendrier McEliece
 * implementation, LGPL 2.1.
 */

#ifndef GF_H
#define GF_H

#include <stdint.h>

typedef uint16_t gf16_t;

extern uint32_t gf_extension_degree, gf_cardinality, gf_multiplicative_order;
extern gf16_t *gf_log_t, *gf_exp_t;

#define gf_extd()		gf_extension_degree
#define gf_card()		gf_cardinality
#define gf_ord()		gf_multiplicative_order

#define gf_unit()		1
#define gf_zero()		0

#define gf_add(x, y)		((x) ^ (y))
#define gf_sub(x, y)		((x) ^ (y))
#define gf_exp(i)		gf_exp_t[i] /* alpha^i */
#define gf_log(x)		gf_log_t[x] /* return i when x=alpha^i */

#define gf_zdo(t, func)		((t) ? (func) : 0)

/**
 * residual modulo q-1
 *  when -q < d < 0, we get (q-1+d)
 *  when 0 <= d < q, we get (d)
 *  when q <= d < 2q-1, we get (d-q+1)
 * we obtain a value between 0 and (q-1) included, the class of 0 is
 * represented by 0 or q-1 (this is why we write
 * _K->exp[q-1] = _K->exp[0] = 1)
 */
#define _gf_modq_1(d)		(((d) & gf_ord()) + ((d) >> gf_extd()))

#define gf_mul_fast(x, y)	gf_zdo(y, gf_exp[_gf_modq_1(gf_log[x] + gf_log[y])])
#define gf_mul(x, y)		gf_zdo(x, gf_mul_fast(x, y))
#define gf_square(x)		gf_zdo(x, gf_exp[_gf_modq_1(gf_log[x] << 1)])
#define gf_sqrt(x)		gf_zdo(x, gf_exp[_gf_modq_1(gf_log[x] << (gf_extd() - 1))])
#define gf_div(x, y)		gf_zdo(x, gf_exp[_gf_modq_1(gf_log[x] - gf_log[y])])
#define gf_inv(x)		gf_exp[gf_ord() - gf_log[x]]

extern void gf_init(int extdeg);
extern gf16_t gf_rand(int (*rnd_u8)(void));
extern gf16_t gf_pow(gf16_t x, int i);

#endif /* GF_H */
