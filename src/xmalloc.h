/*
 * transsip - the telephony network
 * By Daniel Borkmann <daniel@transsip.org>
 * Copyright 2011, 2012 Daniel Borkmann <dborkma@tik.ee.ethz.ch>,
 * Swiss federal institute of technology (ETH Zurich)
 * Subject to the GPL, version 2.
 */

#ifndef XMALLOC_H
#define XMALLOC_H

#include "built-in.h"

extern __hidden void *xmalloc(size_t size);
extern __hidden void *xzmalloc(size_t size);
extern __hidden void *xmalloc_aligned(size_t size, size_t alignment);
extern __hidden void *xrealloc(void *ptr, size_t nmemb, size_t size);
extern __hidden void xfree(void *ptr);
extern __hidden char *xstrdup(const char *str);

#endif /* XMALLOC_H */
