#ifndef CHAINS_H
#define CHAINS_H

#include "notifier.h"

#define CALL_STATE_MACHINE_CHANGED		1

# define CALL_STATE_MACHINE_IDLE		0
# define CALL_STATE_MACHINE_CALLOUT		1
# define CALL_STATE_MACHINE_CALLIN		2
# define CALL_STATE_MACHINE_SPEAKING		3

extern void init_call_notifier(void);
extern int register_call_notifier(struct event_block *block);
extern int register_call_notifier_once(struct event_block *block);
extern int unregister_call_notifier(struct event_block *block);
extern int call_notifier_exec(unsigned long event, const void *arg);

#endif
