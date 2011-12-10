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
#define FILE_ETCDIR	"/etc/transsip/"
#define ENTROPY_SOURCE  "/dev/random"
#define MAX_MSG         1500
#define SAMPLING_RATE   48000
#define FRAME_SIZE      256
#define PACKETSIZE      43
#define CHANNELS        1

extern int get_port(void);
extern char *get_stun_server(void);

#endif /* CONF_H */
