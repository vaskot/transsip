/*
 * transsip - the telephony network
 * By Daniel Borkmann <daniel@transsip.org>
 * Copyright 2011 Daniel Borkmann <dborkma@tik.ee.ethz.ch>,
 * Swiss federal institute of technology (ETH Zurich)
 * Subject to the GPL, version 2.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <celt/celt.h>
#include <speex/speex_jitter.h>
#include <speex/speex_echo.h>
#include <sched.h>

#include "conf.h"
#include "compiler.h"
#include "die.h"
#include "alsa.h"
#include "xmalloc.h"

extern void enter_shell_loop(void);

#define MAX_MSG		1500
#define SAMPLING_RATE	48000
#define FRAME_SIZE	256
#define PACKETSIZE	43
#define CHANNELS	1

static pthread_t ptid;

static char *port = "30111";
static char *stun_server = "stunserver.org";
static char *alsadev = "plughw:0,0";

extern sig_atomic_t quit;
sig_atomic_t did_stun = 0;
static sig_atomic_t incoming_call = 0;

static struct pollfd *pfds = NULL;
static struct alsa_dev *dev = NULL;
static CELTEncoder *encoder = NULL;
static CELTDecoder *decoder = NULL;
static JitterBuffer *jitter = NULL;
static SpeexEchoState *echostate = NULL;

static void do_call_duplex(void)
{
#if 0
	int ret, recv_started = 0;

	alsa_start(dev);
	while (likely(!quit)) {
		ret = poll(pfds, nfds + 1, -1);
		if (ret < 0)
			panic("Poll returned with %d!\n", ret);

		/* Received packets */
		if (pfds[nfds].revents & POLLIN) {
			n = recv(sd, msg, MAX_MSG, 0);
			int recv_timestamp = ((int*) msg)[0];

			JitterBufferPacket packet;
			packet.data = msg + 4;
			packet.len = n - 4;
			packet.timestamp = recv_timestamp;
			packet.span = FRAME_SIZE;
			packet.sequence = 0;

			jitter_buffer_put(jitter, &packet);
			recv_started = 1;
		}

		/* Ready to play a frame (playback) */
		if (alsa_play_ready(dev, pfds, nfds)) {
			short pcm[FRAME_SIZE * CHANNELS];
			if (recv_started) {
				JitterBufferPacket packet;
				/* Get audio from the jitter buffer */
				packet.data = msg;
				packet.len  = MAX_MSG;
				jitter_buffer_tick(jitter);
				jitter_buffer_get(jitter, &packet, FRAME_SIZE,
						  NULL);
				if (packet.len == 0)
					packet.data=NULL;
				celt_decode(dec_state, (const unsigned char *)
					    packet.data, packet.len, pcm);
			} else {
				for (i = 0; i < FRAME_SIZE * CHANNELS; ++i)
					pcm[i] = 0;
			}

			/* Playback the audio and reset the echo canceller
			   if we got an underrun */

			if (alsa_write(dev, pcm, FRAME_SIZE)) 
				speex_echo_state_reset(echo_state);
			/* Put frame into playback buffer */
			speex_echo_playback(echo_state, pcm);
		}

		/* Audio available from the soundcard (capture) */
		if (alsa_cap_ready(dev, pfds, nfds)) {
			short pcm[FRAME_SIZE * CHANNELS],
			      pcm2[FRAME_SIZE * CHANNELS];
			char outpacket[MAX_MSG];

			alsa_read(dev, pcm, FRAME_SIZE);

			/* Perform echo cancellation */
			speex_echo_capture(echo_state, pcm, pcm2);
			for (i = 0; i < FRAME_SIZE * CHANNELS; ++i)
				pcm[i] = pcm2[i];

			celt_encode(enc_state, pcm, NULL, (unsigned char *)
				    (outpacket + 4), PACKETSIZE);

			/* Pseudo header: four null bytes and a 32-bit
			   timestamp */
			((int*)outpacket)[0] = send_timestamp;
			send_timestamp += FRAME_SIZE;

			rc = sendto(sd, outpacket, PACKETSIZE + 4, 0,
				    (struct sockaddr *) &remote_addr,
				    sizeof(remote_addr));
			if (rc < 0)
				panic("cannot send to socket");
		}
	}
#endif
}

void call_out(char *host, char *port)
{
	/* send notify */
	/* wait for accept */
	/* transfer data */

	printf("Trying to call %s:%s ...\n", host, port);
	do_call_duplex();
}

void call_in(int take)
{
	/* lookup addrbook */
	/* send notify */
	/* transfer data */

	barrier();
	if (incoming_call == 0) {
		printf("No incoming call right now!\n");
		return;
	}

	printf("Trying to take incoming call ...\n");
	do_call_duplex();
}

static void *thread(void *null)
{
	int sock = -1, ret, mtu, nfds, tmp;
	struct addrinfo hints, *ahead, *ai;
	CELTMode *mode;

	while (did_stun == 0)
		barrier();

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	hints.ai_flags = AI_PASSIVE;

	ret = getaddrinfo(NULL, port, &hints, &ahead);
	if (ret < 0)
		panic("Cannot get address info!\n");

	for (ai = ahead; ai != NULL && sock < 0; ai = ai->ai_next) {
		sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if (sock < 0)
			continue;
		if (ai->ai_family == AF_INET6) {
			int one = 1;
#ifdef IPV6_V6ONLY
			ret = setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY,
					 &one, sizeof(one));
			if (ret < 0) {
				close(sock);
				sock = -1;
				continue;
			}
#else
			close(sock);
			sock = -1;
			continue;
#endif /* IPV6_V6ONLY */
		}
		mtu = IP_PMTUDISC_DONT;
		setsockopt(sock, SOL_IP, IP_MTU_DISCOVER, &mtu, sizeof(mtu));
		ret = bind(sock, ai->ai_addr, ai->ai_addrlen);
		if (ret < 0) {
			close(sock);
			sock = -1;
			continue;
		}
	}
	freeaddrinfo(ahead);
	if (sock < 0)
		panic("Cannot open socket!\n");

	dev = alsa_open(alsadev, SAMPLING_RATE, CHANNELS, FRAME_SIZE);
	if (!dev)
		panic("Cannot open ALSA device %s!\n", alsadev);

	mode = celt_mode_create(SAMPLING_RATE, FRAME_SIZE, NULL);
	encoder = celt_encoder_create(mode, CHANNELS, NULL);
	decoder = celt_decoder_create(mode, CHANNELS, NULL);

	nfds = alsa_nfds(dev);
	pfds = xmalloc(sizeof(*pfds) * (nfds + 1));
	alsa_getfds(dev, pfds, nfds);
	pfds[nfds].fd = sock;
	pfds[nfds].events = POLLIN;

	jitter = jitter_buffer_init(FRAME_SIZE);
	tmp = FRAME_SIZE;
	jitter_buffer_ctl(jitter, JITTER_BUFFER_SET_MARGIN, &tmp);

	echostate = speex_echo_state_init(FRAME_SIZE, 10 * FRAME_SIZE);
	tmp = SAMPLING_RATE;
	speex_echo_ctl(echostate, SPEEX_ECHO_SET_SAMPLING_RATE, &tmp);

	while (likely(!quit)) {
		ret = poll(&pfds[nfds], 1, -1);
		if (ret < 0)
			panic("Poll returned with %d!\n", ret);
		if (pfds[nfds].revents & POLLIN) {
			/* Check for Auth pkt, if yes, set current caller and notifiy cli */
			/* cli user must call take, that triggers call_in() */
			printf(".\n");
		}
	}

	xfree(pfds);
	alsa_close(dev);
	close(sock);
	pthread_exit(0);
}

static void start_server(void)
{
	int ret = pthread_create(&ptid, NULL, thread, NULL);
	if (ret)
		panic("Cannot create server thread!\n");
}

static void stop_server(void)
{
/*	pthread_join(ptid, NULL);*/
}

int main(int argc, char **argv)
{
	start_server();
	enter_shell_loop();
	stop_server();
	return 0;
}

int get_port(void)
{
	return atoi(port);
}

char *get_stun_server(void)
{
	return stun_server;
}

