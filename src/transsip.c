#include <pthread.h>

#include "die.h"
#include "xutils.h"

extern void enter_shell_loop(int tsocki, int tsocko);
extern void *engine_main(void *arg);

static pthread_t tid;
static struct pipepair pp;

static void start_server(int usocki, int usocko)
{
	pp.i = usocki;
	pp.o = usocko;
	int ret = pthread_create(&tid, NULL, engine_main, &pp);
	if (ret)
		panic("Cannot create server thread!\n");
}

int main(void)
{
	int ret;
	int efd[2], refd[2];

	ret = pipe(efd);
	if (ret < 0)
		panic("Cannot create event fd!\n");
	ret = pipe(refd);
	if (ret < 0)
		panic("Cannot create event fd!\n");

	start_server(efd[0], refd[1]);
	enter_shell_loop(refd[0], efd[1]);

	close(efd[0]);
	close(efd[1]);
	close(refd[0]);
	close(refd[1]);

	return 0;
}
