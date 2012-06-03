/*
 * transsip - the telephony toolkit
 * By Daniel Borkmann <daniel@transsip.org>
 * Copyright 2011, 2012 Daniel Borkmann <dborkma@tik.ee.ethz.ch>
 * Subject to the GPL, version 2.
 */

#ifndef CLICMDS_H
#define CLICMDS_H

#include <readline/readline.h>

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

#endif /* CLICMDS_H */
