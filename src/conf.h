/*
 * transsip - the telephony network
 * By Daniel Borkmann <daniel@transsip.org>
 * Copyright 2011 Daniel Borkmann.
 * Subject to the GPL, version 2.
 */

#ifndef CONF_H
#define CONF_H

#include <unistd.h>

#define FILE_SETTINGS   ".transsip/settings"
#define FILE_CONTACTS   ".transsip/contacts"
#define FILE_PRIVKEY    ".transsip/priv.key"
#define FILE_PUBKEY     ".transsip/pub.key"
#define FILE_USERNAME   ".transsip/username"
#define ENTROPY_SOURCE  "/dev/random"

extern int get_port(void);
extern char *get_stun_server(void);

#define TUNBUFF_SIZ	(3 * getpagesize())

#endif /* CONF_H */
