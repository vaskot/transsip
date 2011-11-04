/*
 * transsip - the telephony network
 * By Daniel Borkmann <daniel@transsip.org>
 * Copyright 2011 Daniel Borkmann <dborkma@tik.ee.ethz.ch>,
 * Swiss federal institute of technology (ETH Zurich)
 * Subject to the GPL, version 2.
 */

#ifndef SIGNALS_H
#define SIGNALS_H

#include <signal.h>
#include <assert.h>

static inline void register_signal(int signal, void (*handler)(int))
{
	sigset_t block_mask;
	struct sigaction saction;

	assert(handler);

	sigfillset(&block_mask);
	saction.sa_handler = handler;
	saction.sa_mask = block_mask;
	saction.sa_flags = SA_RESTART;

	sigaction(signal, &saction, NULL);
}

#endif /* SIGNALS_H */
