/*
 * transsip - the telephony toolkit
 * By Daniel Borkmann <daniel@transsip.org>
 * Copyright 2011, 2012 Daniel Borkmann <dborkma@tik.ee.ethz.ch>
 * Subject to the GPL, version 2.
 */

#include <stdint.h>
#include <errno.h>

#include "notifier.h"
#include "built_in.h"

int register_event_hook(struct event_block **head,
		        struct event_block *block)
{
	if (!block || !head)
		return -EINVAL;
	if (!block->hook)
		return -EINVAL;

	while ((*head) != NULL) {
		if (block->prio > (*head)->prio)
			break;

		head = &((*head)->next);
	}

	block->next = (*head);
	(*head) = block;

	return 0;
}

int register_event_hook_once(struct event_block **head,
			     struct event_block *block)
{
	if (!block || !head)
		return -EINVAL;
	if (!block->hook)
		return -EINVAL;

	while ((*head) != NULL) {
		if (unlikely(block == (*head)))
			return -EEXIST;

		if (block->prio > (*head)->prio)
			break;

		head = &((*head)->next);
	}

	block->next = (*head);
	(*head) = block;

	return 0;
}

int unregister_event_hook(struct event_block **head,
			  struct event_block *block)
{
	if (!block || !head)
		return -EINVAL;

	while ((*head) != NULL) {
		if (unlikely(block == (*head))) {
			(*head) = block->next;
			break;
		}

		head = &((*head)->next);
	}

	return 0;
}

int call_event_hooks(struct event_block **head, unsigned long event,
		     const void *arg, int *called)
{
	int ret = BLOCK_SUCC_DONE;
	struct event_block *block = *head, *next_block;

	if (!head || !arg)
		return -EINVAL;
	if (called)
		(*called) = 0;

	while (block) {
		next_block = block->next;

		ret = block->hook(block, event, arg);
		if (ret & BLOCK_STOP_CHAIN)
			break;

		if(called)
			(*called)++;

		block = next_block;
	}

	return ret;
}
