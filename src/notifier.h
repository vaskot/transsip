#ifndef NOTIFIER_H
#define NOTIFIER_H

#include <stdint.h>
#include <stdio.h>

#include "locking.h"

#define BLOCK_SUCC_DONE   0
#define BLOCK_STOP_CHAIN  1

enum event_prio {
        PRIO_VERYLOW,
        PRIO_LOW,
        PRIO_MEDIUM,
        PRIO_HIGH,
        PRIO_EXTRA,
};

struct event_block {
	enum event_prio prio;
	int (*hook)(const struct event_block *self, unsigned long event,
		    const void *arg);
	struct event_block *next;
};

struct event_head {
	struct event_block *head;
	struct mutexlock lock;
};

static inline void init_event_head(struct event_head *head)
{
	head->head = NULL;
	mutexlock_init(&head->lock);
}

extern int register_event_hook(struct event_block **head,
			       struct event_block *block);
extern int register_event_hook_once(struct event_block **head,
				    struct event_block *block);
extern int unregister_event_hook(struct event_block **head,
				 struct event_block *block);
extern int call_event_hooks(struct event_block **head, unsigned long event,
			    const void *arg, int *called);

#endif /* NOTIFIER_H */
