/*
 * transsip - the telephony network
 * By Daniel Borkmann <daniel@transsip.org>
 * Copyright 2011 Daniel Borkmann <dborkma@tik.ee.ethz.ch>,
 * Swiss federal institute of technology (ETH Zurich)
 * Subject to the GPL, version 2.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "die.h"
#include "compiler.h"
#include "xmalloc.h"
#include "version.h"

static void help(void)
{
	printf("\ntranssip %s, elliptic-curve-crypto-based p2p telephony network\n",
	       VERSION_STRING);
	printf("http://www.transsip.org\n\n");
	printf("Usage: transsip [options]\n");
	printf("Options:\n");
	printf("  -v|--version            Print version\n");
	printf("  -h|--help               Print this help\n");
	printf("\n");
	printf("Example:\n");
	printf("\n");
	printf("Note:\n");
	printf("  There is no default port specified, so that you are forced\n");
	printf("  to select your own! For client/server status messages see syslog!\n");
	printf("\n");
	printf("Secret ingredient: 7647-14-5\n");
	printf("\n");
	printf("Please report bugs to <bugs@transsip.org>\n");
	printf("Copyright (C) 2011 Daniel Borkmann <dborkma@tik.ee.ethz.ch>,\n");
	printf("License: GNU GPL version 2\n");
	printf("This is free software: you are free to change and redistribute it.\n");
	printf("There is NO WARRANTY, to the extent permitted by law.\n\n");
	die();
}

static void version(void)
{
	printf("\ntranssip %s, elliptic-curve-crypto-based p2p telephony network\n",
	       VERSION_STRING);
	printf("Build: %s\n", BUILD_STRING);
	printf("http://www.transsip.org\n\n");
	printf("Please report bugs to <bugs@transsip.org>\n");
	printf("Copyright (C) 2011 Daniel Borkmann <dborkma@tik.ee.ethz.ch>,\n");
	printf("License: GNU GPL version 2\n");
	printf("This is free software: you are free to change and redistribute it.\n");
	printf("There is NO WARRANTY, to the extent permitted by law.\n\n");
	die();
}

int main(int argc, char **argv)
{
	return 0;
}
