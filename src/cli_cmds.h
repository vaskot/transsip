/*
 * transsip - the telephony network
 * By Daniel Borkmann <daniel@transsip.org>
 * Copyright 2011 Daniel Borkmann <dborkma@tik.ee.ethz.ch>,
 * Swiss federal institute of technology (ETH Zurich)
 * Subject to the GPL, version 2.
 */

#ifndef CLI_CMDS_H
#define CLI_CMDS_H

#include <readline/readline.h>

#define MAX_MENU_ELEMS 100

extern int cmd_help(char *args);
extern int cmd_quit(char *args);
extern int cmd_stat(char *args);
extern int cmd_call_ip(char *args);

struct shell_cmd {
	char *name;
	rl_icpfunc_t *callback;
	char *doc;
	struct shell_cmd *sub_cmd;
};

static struct shell_cmd call_node[] = {
	{ "ip", cmd_call_ip, "Call an IP directly",  NULL, },
	{ "peer", cmd_help, "Call a peer throught transsip DHT",  NULL, },
	{ NULL, NULL, NULL, NULL, },
};

static struct shell_cmd show_node[] = {
	{ "settings", cmd_help, "Show settings", NULL, },
	{ "pubkey", cmd_help, "Show my public key", NULL, },
	{ "contacts", cmd_help, "Show my contacts", NULL, },
	{ NULL, NULL, NULL, NULL, },
};

static struct shell_cmd import_node[] = {
	{ "contact",  cmd_help, "Import a contact user/pubkey",  NULL, },
	{ NULL, NULL, NULL, NULL, },
};

static struct shell_cmd cmd_tree[] = {
	{ "help", cmd_help, "Show help", NULL, },
	{ "quit", cmd_quit, "Exit transsip shell", NULL, },
	{ "call", NULL, "Perform a call", call_node, },
	{ "take", cmd_help, "Take a call", NULL, },
	{ "show", NULL, "Show information", show_node, },
	{ "import", NULL, "Import things", import_node, },
	{ NULL, NULL, NULL, NULL, },
};

#endif /* CLI_CMDS_H */

