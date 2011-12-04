/*
 * transsip - the telephony network
 * By Daniel Borkmann <daniel@transsip.org>
 * Copyright 2011 Daniel Borkmann.
 * Subject to the GPL, version 2.
 */

#ifndef MERSENNE_TWISTER_H
#define MERSENNE_TWISTER_H

extern void mt_init_by_seed_rand(unsigned long s);
extern void mt_init_by_seed_time(void);
extern void mt_init_by_seed_array(unsigned long key[], int len);
extern void mt_init_by_seed_rand_array(void);
extern void mt_init_by_random_device(void);
extern unsigned long mt_rand_int32(void);
extern long mt_rand_int31(void);
extern double mt_rand_real1(void);
extern double mt_rand_real2(void);
extern double mt_rand_real3(void);
extern double mt_rand_res53(void);

#endif /* MERSENNE_TWISTER_H */
