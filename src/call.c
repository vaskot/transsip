/*
 * transsip - the telephony network
 * By Daniel Borkmann <daniel@transsip.org>
 * Copyright 2011 Daniel Borkmann <dborkma@tik.ee.ethz.ch>,
 * Swiss federal institute of technology (ETH Zurich)
 * Subject to the GPL, version 2.
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "cli_cmds.h"
#include "alsa.h"
#include "die.h"
#include "xmalloc.h"

int cmd_call_ip(char *arg)
{
	int argc, i;
	char **argv = strntoargv(arg, strlen(arg), &argc);

	if (argc != 2) {
		printf("Missing arguments: call ip <ipv4/ipv6/host> <port>\n");
		return -EINVAL;
	}

	for (i = 0; i < argc; ++i) {
		printf("%d: %s\n", i, argv[i]);
	}

	xfree(argv);
	return 0;
}

