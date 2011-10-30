/*
 * netyack
 * By Daniel Borkmann <daniel@netyack.org>
 * Copyright 2011 Daniel Borkmann.
 * Subject to the GPL, version 2.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <jack/jack.h>
#include <celt/celt.h>
#include <speex/speex_jitter.h>
#include <speex/speex_echo.h>

#include "die.h"
#include "compiler.h"
#include "xmalloc.h"

static jack_port_t *input, *output;
static jack_client_t *client;

static int jack_process_phys_frames(jack_nframes_t nframes, void *arg)
{
	jack_default_audio_sample_t *in, *out;
	in = jack_port_get_buffer(input, nframes);
	out = jack_port_get_buffer(output, nframes);
	memcpy(out, in, sizeof(*in) * nframes);
	return 0;
}

static int jack_process_new_net_frames(jack_nframes_t nframes, void *arg)
{
}

static void jack_shutdown(void *arg)
{
	die();
}

static void jack_setup(const char *name)
{
	int ret;
	const char **ports;
	jack_options_t options = JackNoStartServer;
	jack_status_t status;

	client = jack_client_open(name, options, &status, NULL);
	if (!client) {
		whine("Opening JACK client failed (0x%2.0x)\n", status);
		if (status & JackServerFailed)
			whine("JACK server not running?\n");
		die();
	}

	jack_set_process_callback(client, jack_process_phys_frames, 0);
	jack_on_shutdown(client, jack_shutdown, 0);

	input = jack_port_register(client, "input",
				   JACK_DEFAULT_AUDIO_TYPE,
				   JackPortIsInput, 0);
	output = jack_port_register(client, "output",
				    JACK_DEFAULT_AUDIO_TYPE,
				    JackPortIsOutput, 0);
	if (!input || !output)
		panic("No more JACK ports available!\n");

	ret = jack_activate(client);
	if (ret)
		panic("Cannot activate JACK client!\n");

	ports = jack_get_ports(client, NULL, NULL,
			       JackPortIsPhysical | JackPortIsOutput);
	if (!ports)
		panic("No physical capture ports available!\n");

	ret = jack_connect(client, ports[0], jack_port_name(input));
	free(ports);
	if (ret)
		panic("Cannot connect input ports!\n");
	
	ports = jack_get_ports(client, NULL, NULL,
			       JackPortIsPhysical | JackPortIsInput);
	if (!ports)
		panic("No physical playback ports available!\n");

	ret = jack_connect(client, jack_port_name(output), ports[0]);
	free(ports);
	if (ret)
		panic("Cannot connect output ports!\n");

	info("plugged to jack\n");
}

int main(int argc, char **argv)
{
	jack_setup(argv[0]);

	sleep(-1);

	jack_client_close(client);
	return 0;
}
