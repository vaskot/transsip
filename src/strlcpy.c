/*
 * transsip - the telephony network
 * By Daniel Borkmann <daniel@transsip.org>
 * Copyright 2011 Daniel Borkmann <dborkma@tik.ee.ethz.ch>,
 * Swiss federal institute of technology (ETH Zurich)
 * Subject to the GPL, version 2.
 * strlcpy: Copyright (C) 1991, 1992  Linus Torvalds, GPL, version 2
 */

#define _BSD_SOURCE
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "strlcpy.h"
#include "xmalloc.h"
#include "die.h"

size_t strlcpy(char *dest, const char *src, size_t size)
{
	size_t ret = strlen(src);
	if (size) {
		size_t len = (ret >= size) ? size - 1 : ret;
		memcpy(dest, src, len);
		dest[len] = '\0';
	}
	return ret;
}

int slprintf(char *dst, size_t size, const char *fmt, ...)
{
	int ret;
	va_list ap;
	va_start(ap, fmt);
	ret = vsnprintf(dst, size, fmt, ap);
	dst[size - 1] = '\0';
	va_end(ap);
	return ret;
}

char **strntoargv(char *str, size_t len, int *argc)
{
	int done = 0;
	char **argv = NULL;
	if (argc == NULL)
		panic("argc is null!\n");
	*argc = 0;
	if (len <= 1) /* '\0' */
		goto out;
	while (!done) {
		while (len > 0 && *str == ' ') {
			len--;
			str++;
		}
		if (len > 0 && *str != '\0') {
			(*argc)++;
			argv = xrealloc(argv, 1, sizeof(char *) * (*argc));
			argv[(*argc) - 1] = str;
			while (len > 0 && *str != ' ') {
				len--;
				str++;
			}
			if (len > 0 && *str == ' ') {
				len--;
				*str = '\0';
				str++;
			}
		} else {
			done = 1;
		}
	}
out:
	return argv;
}

