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
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <celt/celt.h>
#include <speex/speex_jitter.h>
#include <speex/speex_echo.h>
#include <sched.h>

#include "cli.h"
#include "conf.h"
#include "compiler.h"
#include "die.h"
#include "alsa.h"

static pthread_t ptid;
static char *port = "30111";
static char *stun_server = "stunserver.org";

extern sig_atomic_t quit;
sig_atomic_t did_stun = 0;

void *thread(void *null)
{
	int sock = -1, ret, mtu;
	struct addrinfo hints, *ahead, *ai;

	while (!did_stun)
		;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	hints.ai_flags = AI_PASSIVE;

	ret = getaddrinfo(NULL, port, &hints, &ahead);
	if (ret < 0)
		panic("Cannot get address info!\n");

	for (ai = ahead; ai != NULL && sock < 0; ai = ai->ai_next) {
		sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if (sock < 0)
			continue;
		if (ai->ai_family == AF_INET6) {
			int one = 1;
#ifdef IPV6_V6ONLY
			ret = setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY,
					 &one, sizeof(one));
			if (ret < 0) {
				close(sock);
				sock = -1;
				continue;
			}
#else
			close(sock);
			sock = -1;
			continue;
#endif /* IPV6_V6ONLY */
		}
		mtu = IP_PMTUDISC_DONT;
		setsockopt(sock, SOL_IP, IP_MTU_DISCOVER, &mtu, sizeof(mtu));
		ret = bind(sock, ai->ai_addr, ai->ai_addrlen);
		if (ret < 0) {
			close(sock);
			sock = -1;
			continue;
		}
	}
	freeaddrinfo(ahead);
	if (sock < 0)
		panic("Cannot open socket!\n");

	while (likely(!quit))
		;

	close(sock);
	pthread_exit(0);
}

void start_server(void)
{
	int ret = pthread_create(&ptid, NULL, thread, NULL);
	if (ret)
		panic("Cannot create server thread!\n");
}

void stop_server(void)
{
/*	pthread_join(ptid, NULL);*/
}

int main(int argc, char **argv)
{
	start_server();
	enter_shell_loop();
	stop_server();
	return 0;
}

int get_port(void)
{
	return atoi(port);
}

char *get_stun_server(void)
{
	return stun_server;
}

