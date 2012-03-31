#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <ctype.h>

#include "clicmds.h"
#include "xmalloc.h"
#include "xutils.h"
#include "built-in.h"
#include "die.h"

#define FETCH_LIST  0
#define FETCH_ELEM  1

static int exit_val = 0;

sig_atomic_t quit = 0;

extern sig_atomic_t stun_done;

extern void print_stun_probe(char *server, int sport, int tport);

static void fetch_user(char *user, size_t len)
{
	int ret = getlogin_r(user, len);
	if (ret)
		strlcpy(user, "anon", len);
	user[len - 1] = 0;
}

static void fetch_host(char *host, size_t len)
{
	int ret = gethostname(host, len);
	if (ret)
		strlcpy(host, "local", len);
	host[len - 1] = 0;
}

static void setup_prompt(char *prompt, size_t len)
{
	char user[64], host[64];

	fetch_user(user, sizeof(user));
	fetch_host(host, sizeof(host));

	memset(prompt, 0, len);
	slprintf(prompt, len, "%s@%s> ", user, host);
}

static void find_list(struct shell_cmd **list, const char *token)
{
	int i;
	char *cmd;

	for (i = 0; (cmd = (*list)[i].name); ++i) {
		if (strncmp(cmd, token, min(strlen(token), strlen(cmd))))
			continue;
		if (strlen(cmd) != strlen(token))
			continue;
		if ((*list)[i].sub_cmd != NULL)
			(*list) = (*list)[i].sub_cmd;
		break;
	}
}

static struct shell_cmd *find_elem(struct shell_cmd *list, const char *token)
{
	int i;
	char *cmd;
	struct shell_cmd *elem;

	if (!list || !token)
		return NULL;

	for (i = 0, elem = NULL; (cmd = list[i].name); ++i) {
		if (strncmp(cmd, token, min(strlen(token), strlen(cmd))))
			continue;
		elem = &list[i];
	}

	return elem;
}

static char *get_next_token(char *line, int *off)
{
	int i = *off;
	char *token;

	while (line[i] && isspace(line[i]))
		i++;
	token = line + i;
	while (line[i] && !isspace(line[i]))
		i++;
	if (line[i])
		line[i++] = '\0';
	*off = i;

	return token;
}

static struct shell_cmd *walk_commands(char *line_buffer, int len, int ret)
{
	int off = 0;
	char *token, *line;
	struct shell_cmd *list, *elem = NULL;

	list = cmd_tree;
	line = xmalloc(len + 1);
	strlcpy(line, line_buffer, len + 1);

	while (list && len > 0) {
		token = get_next_token(line, &off);
		if (strlen(token) == 0)
			break;

		find_list(&list, token);
		elem = find_elem(list, token);
		if (elem) {
			if (strlen(elem->name) == strlen(token))
				list = NULL;
			break;
		}
	}

	xfree(line);
	return (ret == FETCH_ELEM ? elem : list);
}

static char **__cmd_complete_line(const char *text, char *line_buffer, int point)
{
	int i, j, wlen;
	char *word, *cmd, **list;
	struct shell_cmd *curr;

	word = line_buffer + point - strlen(text);
	curr = walk_commands(line_buffer, strlen(line_buffer), FETCH_LIST);
	if (!curr)
		return NULL;

	wlen = strlen(word);
	list = xzmalloc(MAX_MENU_ELEMS * sizeof(*list));

	for (i = j = 0; (cmd = curr[j].name); ++j)
		if (strncmp(cmd, word, min(wlen, strlen(cmd))) == 0)
			list[i++] = xstrdup(curr[j].name);

	return list;
}

static char *cmd_line_completion(const char *text, int matches,
				 char *line_buffer, int point)
{
	static char **list = NULL;
	static int i;
	char *curr = NULL;

	if (matches == 0) {
		if (list) {
			xfree(list);
			list = NULL;
		}

		i = 0;
		list = __cmd_complete_line(text, line_buffer, point);
	}

	if (list) {
		curr = list[i];
		if (curr)
			++i;
	}

	return curr;
}

char *cmd_completion(const char *text, int matches)
{
	return cmd_line_completion(text, matches, rl_line_buffer, rl_point);
}

static int process_command(char *line)
{
	int i = 0;
	char *token, *ptr;
	struct shell_cmd *curr;

	curr = walk_commands(line, strlen(line), FETCH_ELEM);
	if (!curr || curr->callback == NULL)
		goto err;

	ptr = strstr(line, curr->name);
	if (ptr == NULL)
		goto err;
	ptr += strlen(curr->name);

	while (ptr[i] && isspace(ptr[i]))
		i++;
	token = ptr + i;

	return curr->callback(token);
err:
	printf("Ooops, bad command! Try `help` for more information.\n");
	return -EINVAL;
}

void clear_term(int signal)
{
	if (rl_end)
		rl_kill_line(-1, 0);
	rl_crlf();
	rl_refresh_line(0, 0);
	rl_free_line_state();
}

static void setup_readline(void)
{
	rl_readline_name = "transsip";
	rl_completion_entry_function = cmd_completion;
	rl_catch_signals = 0;
	rl_catch_sigwinch = 1;
	rl_set_signals();

	register_signal(SIGINT, clear_term);
}

static char *strip_white(char *line)
{
	char *l, *t;

	for (l = line; isspace(*l); l++)
		;
	if (*l == 0)
		return l;

	t = l + strlen(l) - 1;
	while (t > l && isspace(*t))
		t--;
	*++t = '\0';

	return l;
}

static int tsocki, tsocko;

void enter_shell_loop(int __tsocki, int __tsocko)
{
	char *prompt;
	char *line, *cmd;
	size_t prompt_len = 256;

	tsocki = __tsocki;
	tsocko = __tsocko;

	prompt = xzmalloc(prompt_len);
	setup_prompt(prompt, prompt_len);
	setup_readline();

	printf("\n%s%s%s shell\n\n", colorize_start(bold),
	       PROGNAME_STRING " " VERSION_STRING, colorize_end());
	print_stun_probe("stunserver.org", 3478, 30111);
	stun_done = 1;
	fflush(stdout);

	while (!quit) {
		line = readline(prompt);
		if (!line) {
			printf("\n");
			break;
		}

		cmd = strip_white(line);
		if (*cmd) {
			add_history(cmd);
			exit_val = process_command(cmd);
		}

		xfree(line);
	}

	xfree(prompt);
	printf("\n");
}

int cmd_help(char *args)
{
	struct shell_cmd *cmd;
	int i, entries = (sizeof(cmd_tree) / sizeof(cmd_tree[0])) - 1;

	if (!*args) {
		for (i = 0; i < entries; ++i)
			printf("%s - %s\n", cmd_tree[i].name, cmd_tree[i].doc);
		return 0;
	}

	cmd = walk_commands(args, strlen(args), FETCH_ELEM);
	if (!cmd || !cmd->doc || !cmd->name)
		return -EINVAL;

	printf("%s - %s\n", cmd->name, cmd->doc);
	return 0;
}

int cmd_quit(char *args)
{
	quit = 1;
	return 0;
}

int cmd_stat(char *args)
{
	printf("%d\n", exit_val);
	return 0;
}

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
