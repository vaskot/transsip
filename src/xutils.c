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

int open_or_die(const char *file, int flags)
{
	int ret = open(file, flags);
	if (ret < 0)
		panic("Open error!!\n");
	return ret;
}

int open_or_die_m(const char *file, int flags, mode_t mode)
{
	int ret = open(file, flags, mode);
	if (ret < 0)
		panic("Open error!!\n");
	return ret;
}

ssize_t read_or_die(int fd, void *buf, size_t len)
{
	ssize_t ret = read(fd, buf, len);
	if (ret < 0) {
		if (errno == EPIPE)
			exit(EXIT_SUCCESS);
		panic("Read error!!\n");
	}

	return ret;
}

ssize_t write_or_die(int fd, const void *buf, size_t len)
{
	ssize_t ret = write(fd, buf, len);
	if (ret < 0) {
		if (errno == EPIPE)
			exit(EXIT_SUCCESS);
		panic("Write error!!\n");
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
