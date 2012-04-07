/*
 * transsip - the telephony toolkit
 * By Daniel Borkmann <daniel@transsip.org>
 * Copyright 2011, 2012 Daniel Borkmann <dborkma@tik.ee.ethz.ch>,
 * Swiss federal institute of technology (ETH Zurich)
 * Subject to the GPL, version 2.
 */

#include <errno.h>

#include "clicmds.h"
#include "xutils.h"
#include "xmalloc.h"
#include "die.h"

static int tsocki, tsocko;

int cmd_call(char *arg)
{
	int argc;
	ssize_t ret;
	char **argv = strntoargv(arg, strlen(arg), &argc);
	struct cli_pkt cpkt;

	if (argc != 2) {
		whine("Missing arguments: call <ipv4/ipv6/host> <port>\n");
		xfree(argv);
		return -EINVAL;
	}
	if (strlen(argv[0]) > ADDRSIZ || strlen(argv[1]) > PORTSIZ) {
		whine("Arguments too long!\n");
		xfree(argv);
		return -EINVAL;
	}

	memset(&cpkt, 0, sizeof(cpkt));
	cpkt.ring = 1;
	strlcpy(cpkt.address, argv[0], sizeof(cpkt.address));
	strlcpy(cpkt.port, argv[1], sizeof(cpkt.port));

	ret = write(tsocko, &cpkt, sizeof(cpkt));
	ret = write(tsocko, &cpkt, sizeof(cpkt)); //XXX
	if (ret != sizeof(cpkt)) {
		whine("Error notifying thread!\n");
		return -EIO;
	}

	printf("Calling ... hang up the line with: hangup\n");

	xfree(argv);
	return 0;
}

int cmd_hangup(char *arg)
{
	ssize_t ret;
	struct cli_pkt cpkt;

	memset(&cpkt, 0, sizeof(cpkt));
	cpkt.fin = 1;

	ret = write(tsocko, &cpkt, sizeof(cpkt));
	if (ret != sizeof(cpkt)) {
		whine("Error notifying thread!\n");
		return -EIO;
	}

	return 0;
}

int cmd_take(char *arg)
{
	ssize_t ret;
	struct cli_pkt cpkt;

	memset(&cpkt, 0, sizeof(cpkt));
	cpkt.take = 1;

	ret = write(tsocko, &cpkt, sizeof(cpkt));
	if (ret != sizeof(cpkt)) {
		whine("Error notifying thread!\n");
		return -EIO;
	}

	return 0;
}

void init_cli_cmds(int ti, int to)
{
	tsocki = ti;
	tsocko = to;
}
