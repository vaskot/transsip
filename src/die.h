/*
 * transsip - the telephony network
 * By Daniel Borkmann <daniel@transsip.org>
 * Copyright 2011 Daniel Borkmann <dborkma@tik.ee.ethz.ch>,
 * Swiss federal institute of technology (ETH Zurich)
 * Subject to the GPL, version 2.
 */

#ifndef DIE_H
#define DIE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

static inline void error_and_die(int status, char *msg, ...)
{
	va_list vl;
	va_start(vl, msg);
	vfprintf(stderr, msg, vl);
	va_end(vl);
	exit(status);
}

static inline void die(void)
{
	exit(EXIT_FAILURE);
}

static inline void panic(char *msg, ...)
{
	va_list vl;
	va_start(vl, msg);
	vfprintf(stderr, msg, vl);
	va_end(vl);
	die();
}

static inline void whine(char *msg, ...)
{
	va_list vl;
	va_start(vl, msg);
	vfprintf(stderr, msg, vl);
	va_end(vl);
}

static inline void info(char *msg, ...)
{
	va_list vl;
	va_start(vl, msg);
	vfprintf(stdout, msg, vl);
	va_end(vl);
}

#ifdef _DEBUG_
static inline void debug(char *msg, ...)
{
	va_list vl;
	va_start(vl, msg);
	vfprintf(stderr, msg, vl);
	va_end(vl);

	fflush(stderr);
}
#else
static inline void debug(char *msg, ...)
{
	/* NOP */
}
#endif /* _DEBUG_ */
#endif /* DIE_H */
