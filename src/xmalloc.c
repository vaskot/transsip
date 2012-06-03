/*
 * transsip - the telephony toolkit
 * By Daniel Borkmann <daniel@transsip.org>
 * Copyright 2011, 2012 Daniel Borkmann <dborkma@tik.ee.ethz.ch>
 * Subject to the GPL, version 2.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>

#include "built_in.h"
#include "xmalloc.h"
#include "die.h"
#include "xutils.h"

__hidden void *xmalloc(size_t size)
{
	void *ptr;

	if (size == 0)
		panic("xmalloc: zero size\n");

	ptr = malloc(size);
	if (ptr == NULL)
		panic("xmalloc: out of memory (allocating %zu bytes)\n", size);

	return ptr;
}

__hidden void *xzmalloc(size_t size)
{
	void *ptr;

	if (size == 0)
		panic("xzmalloc: zero size\n");

	ptr = malloc(size);
	if (ptr == NULL)
		panic("xzmalloc: out of memory (allocating %zu bytes)\n", size);

	memset(ptr, 0, size);

	return ptr;
}

__hidden void *xmalloc_aligned(size_t size, size_t alignment)
{
	int ret;
	void *ptr;

	if (size == 0)
		panic("xmalloc_aligned: zero size\n");

	ret = posix_memalign(&ptr, alignment, size);
	if (ret != 0)
		panic("xmalloc_aligned: out of memory (allocating %zu bytes)\n",
		      size);

	return ptr;
}

__hidden void *xrealloc(void *ptr, size_t nmemb, size_t size)
{
	void *new_ptr;
	size_t new_size = nmemb * size;

	if (unlikely(new_size == 0))
		panic("xrealloc: zero size\n");

	if (unlikely(((size_t) ~0) / nmemb < size))
		panic("xrealloc: nmemb * size > SIZE_T_MAX\n");

	if (ptr == NULL)
		new_ptr = malloc(new_size);
	else
		new_ptr = realloc(ptr, new_size);

	if (unlikely(new_ptr == NULL))
		panic("xrealloc: out of memory (new_size %zu bytes)\n",
		      new_size);

	return new_ptr;
}

__hidden void xfree(void *ptr)
{
	if (ptr == NULL)
		panic("xfree: NULL pointer given as argument\n");

	free(ptr);
}

__hidden char *xstrdup(const char *str)
{
	size_t len;
	char *cp;

	len = strlen(str) + 1;
	cp = xmalloc(len);
	strlcpy(cp, str, len);

	return cp;
}
