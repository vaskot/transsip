/*
 * transsip - the telephony network
 * By Daniel Borkmann <daniel@transsip.org>
 * Copyright 2011 Daniel Borkmann.
 * Subject to the GPL, version 2.
 * strlcpy: Copyright (C) 1991, 1992  Linus Torvalds, GPL, version 2
 */

#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include "xutils.h"
#include "xmalloc.h"
#include "die.h"

extern sig_atomic_t quit;

void fsync_or_die(int fd, const char *msg)
{
	if (fsync(fd) < 0)
		puke_and_die(EXIT_FAILURE, "%s: fsync error", msg);
}

int open_or_die(const char *file, int flags)
{
	int ret = open(file, flags);
	if (ret < 0)
		puke_and_die(EXIT_FAILURE, "Open error");
	return ret;
}

int open_or_die_m(const char *file, int flags, mode_t mode)
{
	int ret = open(file, flags, mode);
	if (ret < 0)
		puke_and_die(EXIT_FAILURE, "Open error");
	return ret;
}

ssize_t read_or_die(int fd, void *buf, size_t len)
{
	ssize_t ret = read(fd, buf, len);
	if (ret < 0) {
		if (errno == EPIPE)
			exit(EXIT_SUCCESS);
		puke_and_die(EXIT_FAILURE, "Read error");
	}

	return ret;
}

ssize_t read_exact(int fd, void *buf, size_t len, int mayexit)
{
	register ssize_t num = 0, written;

	while (len > 0 && !quit) {
		if ((written = read(fd, buf, len)) < 0) {
			if (errno == EAGAIN && num > 0)
				continue;
			if (mayexit)
				return -1;
			else
				continue;
		}

		if (!written)
			return 0;

		len -= written;
		buf += written;
		num += written;
	}

	return num;
}

ssize_t write_exact(int fd, void *buf, size_t len, int mayexit)
{
	register ssize_t num = 0, written;

	while (len > 0 && !quit) {
		if ((written = write(fd, buf, len)) < 0) {
			if (errno == EAGAIN && num > 0)
				continue;
			if (mayexit)
				return -1;
			else
				continue;
		}

		if (!written)
			return 0;

		len -= written;
		buf += written;
		num += written;
	}

	return num;
}

ssize_t write_or_die(int fd, const void *buf, size_t len)
{
	ssize_t ret = write(fd, buf, len);
	if (ret < 0) {
		if (errno == EPIPE)
			exit(EXIT_SUCCESS);
		puke_and_die(EXIT_FAILURE, "Write error");
	}

	return ret;
}

ssize_t write_or_whine_pipe(int fd, const void *buf, size_t len,
			    const char *msg)
{
	ssize_t ret = write(fd, buf, len);
	if (ret < 0) {
		if (errno == EPIPE)
			exit(0);
		whine("%s: write error (%s)!\n", msg, strerror(errno));
		return 0;
	}

	return ret;
}

ssize_t write_or_whine(int fd, const void *buf, size_t len,
		       const char *msg)
{
	ssize_t ret = write(fd, buf, len);
	if (ret < 0) {
		whine("%s: write error (%s)!\n", msg, strerror(errno));
		return 0;
	}

	return ret;
}

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

