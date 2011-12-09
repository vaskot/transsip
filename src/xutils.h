/*
 * transsip - the telephony network
 * By Daniel Borkmann <daniel@transsip.org>
 * Copyright 2011 Daniel Borkmann.
 * Subject to the GPL, version 2.
 */

#ifndef XUTILS_H
#define XUTILS_H

#include <sys/types.h>

extern void fsync_or_die(int fd, const char *msg);
extern int open_or_die(const char *file, int flags);
extern int open_or_die_m(const char *file, int flags, mode_t mode);
extern ssize_t read_or_die(int fd, void *buf, size_t count);
extern ssize_t read_exact(int fd, void *buf, size_t len, int mayexit);
extern ssize_t write_exact(int fd, void *buf, size_t len, int mayexit);
extern ssize_t write_or_die(int fd, const void *buf, size_t count);
extern ssize_t write_or_whine_pipe(int fd, const void *buf, size_t len,
				   const char *msg);
extern ssize_t write_or_whine(int fd, const void *buf, size_t len,
			      const char *msg);
extern size_t strlcpy(char *dest, const char *src, size_t size);
extern int slprintf(char *dst, size_t size, const char *fmt, ...);
extern char **strntoargv(char *str, size_t len, int *argc);

#endif /* XUTILS_H */
