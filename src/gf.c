/*
 * transsip - the telephony toolkit
 * By Daniel Borkmann <daniel@transsip.org>
 * Copyright 2012 Daniel Borkmann <dborkma@tik.ee.ethz.ch>
 * Subject to the GPL, version 2.
 * Based on Bhaskar Biswas and Nicolas Sendrier McEliece
 * implementation, LGPL 2.1.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "gf.h"
#include "die.h"
#include "xmalloc.h"

/* this is our primary consideration....think about changing!? */
#define MAX_EXT_DEG	16

static const uint32_t prim_poly[MAX_EXT_DEG + 1] = {
	01,		/* extension degree 0 (!) never used */
	03,		/* extension degree 1 (!) never used */
	07, 		/* extension degree 2 */
	013, 		/* extension degree 3 */
	023, 		/* extension degree 4 */
	045, 		/* extension degree 5 */
	0103, 		/* extension degree 6 */
	0203, 		/* extension degree 7 */
	0435, 		/* extension degree 8 */
	01041, 		/* extension degree 9 */
	02011,		/* extension degree 10 */
	04005,		/* extension degree 11 */
	010123,		/* extension degree 12 */
	020033,		/* extension degree 13 */
	042103,		/* extension degree 14 */
	0100003,	/* extension degree 15 */
	0210013		/* extension degree 16 */
};

uint32_t gf_extension_degree, gf_cardinality, gf_multiplicative_order;

gf16_t *gf_log_t, *gf_exp_t;

static int init_done = 0;

/* construct the table gf_exp[i]=alpha^i */
static void gf_init_exp_table(void)
{
	int i;

	gf_exp_t = xzmalloc((1 << gf_extd()) * sizeof(*gf_exp_t));
	gf_exp_t[0] = 1;
	for (i = 1; i < gf_ord(); ++i) {
		gf_exp_t[i] = gf_exp_t[i - 1] << 1;
		if (gf_exp_t[i - 1] & (1 << (gf_extd() - 1)))
			gf_exp_t[i] ^= prim_poly[gf_extd()];
	}
	/* hack for the multiplication */
	gf_exp_t[gf_ord()] = 1;
}

/* construct the table gf_log[alpha^i]=i */
static void gf_init_log_table(void)
{
	int i;

	gf_log_t = xzmalloc((1 << gf_extd()) * sizeof(*gf_log_t));
	gf_log_t[0] = gf_ord();
	for (i = 0; i < gf_ord() ; ++i)
		gf_log_t[gf_exp_t[i]] = i;
}

void gf_init(int extdeg)
{
	if (extdeg > MAX_EXT_DEG)
		panic("Extension degree %d not implemented !\n", extdeg);
	if (init_done != extdeg) {
		if (init_done) {
			xfree(gf_exp_t);
			xfree(gf_log_t);
		}

		gf_extension_degree = extdeg;
		gf_cardinality = 1 << extdeg;
		gf_multiplicative_order = gf_cardinality - 1;
		gf_init_exp_table();
		gf_init_log_table();

		init_done = extdeg;
	}
}

/* we suppose i >= 0. Par convention 0^0 = 1 */
gf16_t gf_pow(gf16_t x, int i)
{
	if (i == 0)
		return 1;
	else if (x == 0)
		return 0;
	else {
		/* i mod (q-1) */
		while (i >> gf_extd())
			i = (i & (gf_ord())) + (i >> gf_extd());
		i *= gf_log_t[x];
		while (i >> gf_extd())
			i = (i & (gf_ord())) + (i >> gf_extd());
		return gf_exp_t[i];
	}
}

/* u8rnd is a function returning a random byte */
gf16_t gf_rand(int (*rnd_u8)(void))
{
	return (rnd_u8() ^ (rnd_u8() << 8)) & gf_ord();
}
