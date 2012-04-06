#ifndef CHAINS_H
#define CHAINS_H

#include "notifier.h"

extern void init_call_notifier(void);
extern int register_call_notifier(struct event_block *block);
extern int register_call_notifier_once(struct event_block *block);
extern int unregister_call_notifier(struct event_block *block);
extern int call_notifier_exec(unsigned long event, const void *arg);

#endif
