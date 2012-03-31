/*
 * transsip - the telephony network
 * By Daniel Borkmann <daniel@transsip.org>
 * Copyright 2011, 2012 Daniel Borkmann <dborkma@tik.ee.ethz.ch>,
 * Swiss federal institute of technology (ETH Zurich)
 * Subject to the GPL, version 2.
 */

#ifndef XUTILS_H
#define XUTILS_H

#include <termios.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "built-in.h"

#define FILE_SETTINGS   ".transsip/settings"
#define FILE_CONTACTS   ".transsip/contacts"
#define FILE_PRIVKEY    ".transsip/priv.key"
#define FILE_PUBKEY     ".transsip/pub.key"
#define FILE_USERNAME   ".transsip/username"

#define FILE_ETCDIR	"/etc/transsip"

#define FILE_BUSY	"busytone.s16"
#define FILE_RING	"ring.s16"
#define FILE_DIAL	"dialtone.s16"

#define USERSIZ		64
#define ADDRSIZ		256
#define PORTSIZ		16

struct pipepair {
	int i, o;
};

struct cli_pkt {
	__extension__ uint16_t ring:1,
			       take:1,
			       fin:1,
			       hold:1,
			       unhold:1,
			       res:11;
	char user[USERSIZ];
	char address[ADDRSIZ];
	char port[PORTSIZ];
} __attribute__((packed));

extern int open_or_die(const char *file, int flags);
extern int open_or_die_m(const char *file, int flags, mode_t mode);
extern ssize_t read_or_die(int fd, void *buf, size_t count);
extern ssize_t write_or_die(int fd, const void *buf, size_t count);
extern size_t strlcpy(char *dest, const char *src, size_t size);
extern int slprintf(char *dst, size_t size, const char *fmt, ...);
extern char **strntoargv(char *str, size_t len, int *argc);

#define __reset                 "0"
#define __bold                  "1"
#define __black                 "30"
#define __red                   "31"
#define __green                 "32"
#define __yellow                "33"
#define __blue                  "34"
#define __magenta               "35"
#define __cyan                  "36"
#define __white                 "37"
#define __on_black              "40"
#define __on_red                "41"
#define __on_green              "42"
#define __on_yellow             "43"
#define __on_blue               "44"
#define __on_magenta            "45"
#define __on_cyan               "46"
#define __on_white              "47"

#define colorize_start(fore)            "\033[" __##fore "m"
#define colorize_start_full(fore, back) "\033[" __##fore ";" __on_##back "m"
#define colorize_end()                  "\033[" __reset "m"

#define colorize_str(fore, text)                                     \
		colorize_start(fore) text colorize_end()
#define colorize_full_str(fore, back, text)                          \
		colorize_start_full(fore, back) text colorize_end()

static inline void register_signal(int signal, void (*handler)(int))
{
	sigset_t block_mask;
	struct sigaction saction;
	sigfillset(&block_mask);
	saction.sa_handler = handler;
	saction.sa_mask = block_mask;
	saction.sa_flags = SA_RESTART;
	sigaction(signal, &saction, NULL);
}

static inline int set_timeout(struct timeval *timeval, unsigned int msec)
{
	if (msec == 0)
		return -EINVAL;

	timeval->tv_sec = 0;
	timeval->tv_usec = 0;

	if (msec < 1000) {
		timeval->tv_usec = msec * 1000;
		return 0;
	}

	timeval->tv_sec = (long) (msec / 1000);
	timeval->tv_usec = (long) ((msec - (timeval->tv_sec * 1000)) * 1000);

	return 0;
}

#endif /* XUTILS_H */
