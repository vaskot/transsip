#ifndef CLICMDS_H
#define CLICMDS_H

#include <readline/readline.h>

#define MAX_MENU_ELEMS 100

extern int cmd_help(char *args);
extern int cmd_quit(char *args);
extern int cmd_stat(char *args);
extern int cmd_call(char *args);
extern int cmd_hangup(char *args);
extern int cmd_take(char *arg);

struct shell_cmd {
	char *name;
	rl_icpfunc_t *callback;
	char *doc;
	struct shell_cmd *sub_cmd;
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
	{ "quit", cmd_quit, "Exit shell", NULL, },
	{ "call", cmd_call, "Perform a call", NULL, },
	{ "hangup", cmd_hangup, "Hangup the current call", NULL, },
	{ "take", cmd_take, "Take a call", NULL, },
	{ "show", NULL, "Show information", show_node, },
	{ "import", NULL, "Import things", import_node, },
	{ NULL, NULL, NULL, NULL, },
};

#endif /* CLICMDS_H */
