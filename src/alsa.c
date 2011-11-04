/*
 * transsip - the telephony network
 * By Daniel Borkmann <daniel@transsip.org>
 * Copyright 2011 Daniel Borkmann <dborkma@tik.ee.ethz.ch>,
 * Swiss federal institute of technology (ETH Zurich)
 * Subject to the GPL, version 2.
 */

/*
 * Copyright (C) 2011 Daniel Borkmann (major cleanups, improvements)
 * Copyright (C) 2004-2006 Jean-Marc Valin
 * Copyright (C) 2006 Commonwealth Scientific and Industrial Research
 *                    Organisation (CSIRO) Australia
 *  
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* Local ALSA as an audio source */

#include "compiler.h"
#include "die.h"
#include "alsa.h"
#include "xmalloc.h"
#include "strlcpy.h"

#define PERIODS	3

struct alsa_dev {
	char *name;
	int channels;
	int period;
	snd_pcm_t *capture_handle;
	snd_pcm_t *playback_handle;
	unsigned int read_fds;
	struct pollfd *read_fd;
	unsigned int write_fds;
	struct pollfd *write_fd;
};

struct alsa_dev *alsa_open(char *devname, unsigned int rate,
			   int channels, int period)
{

}

void alsa_close(struct alsa_dev *dev)
{
	snd_pcm_close(dev->capture_handle);
	snd_pcm_close(dev->playback_handle);

	xfree(dev->name);
	xfree(dev);
}

ssize_t alsa_read(struct alsa_dev *dev, short *pcm, size_t len)
{
}

ssize_t alsa_write(struct alsa_dev *dev, const short *pcm, size_t len)
{
}

int alsa_cap_ready(struct alsa_dev *dev, struct pollfd *pfds,
		   unsigned int nfds)
{
}

int alsa_play_ready(struct alsa_dev *dev, struct pollfd *pfds,
		    unsigned int nfds)
{
}

void alsa_start(struct alsa_dev *dev)
{
}

unsigned int alsa_nfds(struct alsa_dev *dev)
{
	return dev->read_fds + dev->write_fds;
}

void alsa_getfds(struct alsa_dev *dev, struct pollfd *pfds,
		 unsigned int nfds)
{
	int i;

	BUG_ON_(nfds < alsa_nfds(dev), __func__);

	for (i = 0; i < dev->read_fds; ++i)
		pfds[i] = dev->read_fd[i];
	for (i = 0; i < dev->write_fds; ++i)
		pfds[i + dev->read_fds] = dev->write_fd[i];
}

