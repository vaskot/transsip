/*
 * transsip - the telephony network
 * By Daniel Borkmann <daniel@transsip.org>
 * Copyright 2011 Daniel Borkmann <dborkma@tik.ee.ethz.ch>,
 * Swiss federal institute of technology (ETH Zurich)
 * Subject to the GPL, version 2.
 */

#ifndef STUN_H
#define STUN_H

#include <stdint.h>

extern void print_stun_probe(char *server, uint16_t sport, uint16_t tunport);

#endif /* STUN_H */
