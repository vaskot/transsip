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

static void help(void)
{
	printf("\npeerspeak %s, lightweight curve25519-based VoIP phone\n",
	       VERSION_STRING);
	printf("http://www.transsip.org\n\n");
	printf("Usage: peerspeak [options]\n");
	printf("Options:\n");
	printf("  -p|--port <num>         Port number for daemon (mandatory)\n");
	printf("  -c|--call <name>        Call alias from config\n");
	printf("  -t|--stun <server>      Show public IP/Port mapping via STUN\n");
	printf("  -k|--keygen             Generate public/private keypair\n");
	printf("  -x|--export             Export your public data for remote servers\n");
	printf("  -C|--dumpc              Dump parsed clients\n");
	printf("  -S|--dumps              Dump parsed servers\n");
	printf("  -v|--version            Print version\n");
	printf("  -h|--help               Print this help\n");
	printf("\n");
	printf("Example:\n");
	printf("  curvetun --keygen\n");
	printf("  curvetun --export\n");
	printf("  curvetun --port 3244 &\n");
	printf("  curvetun --call john\n");
	printf("\n");
	printf("Note:\n");
	printf("  There is no default port specified, so that you are forced\n");
	printf("  to select your own! For client/server status messages see syslog!\n");
	printf("\n");
	printf("Secret ingredient: 7647-14-5\n");
	printf("\n");
	printf("Please report bugs to <bugs@transsip.org>\n");
	printf("Copyright (C) 2011 Daniel Borkmann <dborkma@tik.ee.ethz.ch>,\n");
	printf("License: GNU GPL version 2\n");
	printf("This is free software: you are free to change and redistribute it.\n");
	printf("There is NO WARRANTY, to the extent permitted by law.\n\n");
	die();
}

static void version(void)
{
	printf("\npeerspeak %s, lightweight curve25519-based VoIP phone\n",
	       VERSION_STRING);
	printf("Build: %s\n", BUILD_STRING);
	printf("http://www.transsip.org\n\n");
	printf("Please report bugs to <bugs@transsip.org>\n");
	printf("Copyright (C) 2011 Daniel Borkmann <dborkma@tik.ee.ethz.ch>,\n");
	printf("License: GNU GPL version 2\n");
	printf("This is free software: you are free to change and redistribute it.\n");
	printf("There is NO WARRANTY, to the extent permitted by law.\n\n");
	die();
}

int main(int argc, char **argv)
{
	jack_setup(argv[0]);

	sleep(-1);

	jack_client_close(client);
	return 0;
}
