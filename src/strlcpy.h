/*
 * transsip - the telephony network
 * By Daniel Borkmann <daniel@transsip.org>
 * Copyright 2011 Daniel Borkmann <dborkma@tik.ee.ethz.ch>,
 * Swiss federal institute of technology (ETH Zurich)
 * Subject to the GPL, version 2.
 */

#ifndef STRLCPY_H
#define STRLCPY_H

extern size_t strlcpy(char *dest, const char *src, size_t size);
extern int slprintf(char *dst, size_t size, const char *fmt, ...);

#endif /* STRLCPY_H */
