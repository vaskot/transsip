/*
 * transsip - the telephony toolkit
 * By Daniel Borkmann <daniel@transsip.org>
 * Copyright 2011, 2012 Daniel Borkmann <dborkma@tik.ee.ethz.ch>,
 * Swiss federal institute of technology (ETH Zurich)
 * Subject to the GPL, version 2.
 */

#include "notifier.h"
#include "locking.h"
#include "call-notifier.h"

static struct event_head call_notifier;

void init_call_notifier(void)
{
	call_notifier.head = NULL;
	mutexlock_init(&call_notifier.lock);
}

int register_call_notifier(struct event_block *block)
{
	int ret;

	mutexlock_lock(&call_notifier.lock);
	ret = register_event_hook(&call_notifier.head, block);
	mutexlock_unlock(&call_notifier.lock);

	return ret;
}

int register_call_notifier_once(struct event_block *block)
{
	int ret;

	mutexlock_lock(&call_notifier.lock);
	ret = register_event_hook_once(&call_notifier.head, block);
	mutexlock_unlock(&call_notifier.lock);

	return ret;
}

int unregister_call_notifier(struct event_block *block)
{
	int ret;

	mutexlock_lock(&call_notifier.lock);
	ret = unregister_event_hook(&call_notifier.head, block);
	mutexlock_unlock(&call_notifier.lock);

	return ret;
}

int call_notifier_exec(unsigned long event, const void *arg)
{
	int ret;

	mutexlock_lock(&call_notifier.lock);
	ret = call_event_hooks(&call_notifier.head, event, arg, NULL);
	mutexlock_unlock(&call_notifier.lock);

	return ret;
}
