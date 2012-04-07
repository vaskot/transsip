/*
 * transsip - the telephony toolkit
 * By Daniel Borkmann <daniel@transsip.org>
 * Copyright 2011, 2012 Daniel Borkmann <dborkma@tik.ee.ethz.ch>,
 * Swiss federal institute of technology (ETH Zurich)
 * Subject to the GPL, version 2.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <celt/celt.h>
#include <speex/speex_jitter.h>
#ifdef HAS_SPEEX_AEC
# include <speex/speex_echo.h>
# include <speex/speex_preprocess.h>
#endif
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <assert.h>

#include "built-in.h"
#include "alsa.h"
#include "die.h"
#include "xmalloc.h"
#include "xutils.h"
#include "call-notifier.h"

#define SAMPLING_RATE	48000
#define FRAME_SIZE	256
#define PACKETSIZE	43
#define MAX_MSG		1500
#define PATH_MAX	512

struct transsip_hdr {
	uint32_t seq;
	__extension__ uint8_t est:1,
			      psh:1,
			      bsy:1,
			      fin:1,
			      res1:4;
} __attribute__((packed));

enum engine_state_num {
	ENGINE_STATE_IDLE = CALL_STATE_MACHINE_IDLE,
	ENGINE_STATE_CALLOUT = CALL_STATE_MACHINE_CALLOUT,
	ENGINE_STATE_CALLIN = CALL_STATE_MACHINE_CALLIN,
	ENGINE_STATE_SPEAKING = CALL_STATE_MACHINE_SPEAKING,
	__ENGINE_STATE_MAX,
};

enum engine_sound_type {
	ENGINE_SOUND_DIAL = 0,
	ENGINE_SOUND_RING,
	ENGINE_SOUND_BUSY,
};

struct engine_state {
	volatile enum engine_state_num state;
	enum engine_state_num (*process)(int, int *, int, int,
					 struct alsa_dev *);
};

#define STATE_MAP_SET(s, f)  {	\
	.state = (s),		\
	.process = (f)		\
}

extern volatile sig_atomic_t quit;

volatile sig_atomic_t stun_done = 0;

static char *alsadev = "plughw:0,0"; //XXX
static char *port = "30111"; //XXX

struct engine_curr {
	int active;
	int sock;
	struct sockaddr addr;
	socklen_t addrlen;
};

static struct engine_curr ecurr;

static void engine_play_file(struct alsa_dev *dev, enum engine_sound_type type)
{
	int fd, nfds;
	char path[PATH_MAX];
	short pcm[FRAME_SIZE];
	struct pollfd *pfds = NULL;

	memset(path, 0, sizeof(path));
	switch (type) {
	case ENGINE_SOUND_DIAL:
		slprintf(path, sizeof(path), "%s/%s", FILE_ETCDIR, FILE_DIAL);
		break;
	case ENGINE_SOUND_BUSY:
		slprintf(path, sizeof(path), "%s/%s", FILE_ETCDIR, FILE_BUSY);
		break;
	case ENGINE_SOUND_RING:
		slprintf(path, sizeof(path), "%s/%s", FILE_ETCDIR, FILE_RING);
		break;
	}

	fd = open(path, O_RDONLY);
	if (fd < 0)
		panic("Cannot open ring file!\n");

	alsa_start(dev);

	nfds = alsa_nfds(dev);
	pfds = xmalloc(sizeof(*pfds) * nfds);

	alsa_getfds(dev, pfds, nfds);

	memset(pcm, 0, sizeof(pcm));
	while (read(fd, pcm, sizeof(pcm)) > 0) {
		poll(pfds, nfds, -1);

		alsa_write(dev, pcm, FRAME_SIZE);
		memset(pcm, 0, sizeof(pcm));

		alsa_read(dev, pcm, FRAME_SIZE);
		memset(pcm, 0, sizeof(pcm));
	}

	alsa_stop(dev);
	close(fd);
	xfree(pfds);
}

static inline void engine_play_ring(struct alsa_dev *dev)
{
	engine_play_file(dev, ENGINE_SOUND_RING);
}

static inline void engine_play_busy(struct alsa_dev *dev)
{
	engine_play_file(dev, ENGINE_SOUND_BUSY);
}

static inline void engine_play_dial(struct alsa_dev *dev)
{
	engine_play_file(dev, ENGINE_SOUND_DIAL);
}

void engine_decode_packet(uint8_t *pkt, size_t len)
{
	struct transsip_hdr *hdr;

	if (len < sizeof(*hdr)) {
		whine("[dbg] pkt too small!\n");
		return;
	}

	hdr = (struct transsip_hdr *) pkt;
	whine("[dbg] packet:\n");
	whine("[dbg]   seq: %d\n", hdr->seq);
	whine("[dbg]   est: %d\n", hdr->est);
	whine("[dbg]   psh: %d\n", hdr->psh);
	whine("[dbg]   bsy: %d\n", hdr->bsy);
	whine("[dbg]   fin: %d\n", hdr->fin);
	whine("[dbg]   res1: %d\n", hdr->res1);
}

static enum engine_state_num engine_do_callout(int ssock, int *csock, int usocki,
					       int usocko, struct alsa_dev *dev)
{
	int one, mtu, tries, i;
	ssize_t ret;
	struct cli_pkt cpkt;
	struct addrinfo hints, *ahead, *ai;
	char msg[MAX_MSG];
	struct pollfd fds[2];
	struct sockaddr raddr;
	socklen_t raddrlen;
	struct transsip_hdr *thdr;

	assert(ecurr.active == 0);

	memset(&cpkt, 0, sizeof(cpkt));
	ret = read(usocki, &cpkt, sizeof(cpkt));
	if (ret != sizeof(cpkt)) {
		whine("Read error from cli!\n");
		return ENGINE_STATE_IDLE;
	}
	if (cpkt.ring == 0) {
		whine("Read no ring flag from cli!\n");
		return ENGINE_STATE_IDLE;
	}

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	hints.ai_flags = AI_NUMERICSERV;

	ret = getaddrinfo(cpkt.address, cpkt.port, &hints, &ahead);
	if (ret < 0) {
		whine("Cannot get address info for %s:%s!\n",
		      cpkt.address, cpkt.port);
		return ENGINE_STATE_IDLE;
	}

	*csock = -1;

	for (ai = ahead; ai != NULL && *csock < 0; ai = ai->ai_next) {
		*csock = socket(ai->ai_family, ai->ai_socktype,
				ai->ai_protocol);
		if (*csock < 0)
			continue;

		ret = connect(*csock, ai->ai_addr, ai->ai_addrlen);
		if (ret < 0) {
			whine("Cannot connect to remote!\n");
			close(*csock);
			*csock = -1;
			continue;
		}

		one = 1;
		setsockopt(*csock, SOL_SOCKET, SO_KEEPALIVE, &one, sizeof(one));

		mtu = IP_PMTUDISC_DONT;
		setsockopt(*csock, SOL_IP, IP_MTU_DISCOVER, &mtu, sizeof(mtu));

		memcpy(&ecurr.addr, ai->ai_addr, ai->ai_addrlen);
		ecurr.addrlen = ai->ai_addrlen;
		ecurr.sock = *csock;
		ecurr.active = 0;
	}
	freeaddrinfo(ahead);

	if (*csock < 0) {
		whine("Cannot connect to server!\n");
		return ENGINE_STATE_IDLE;
	}

	tries = 0;

	memset(msg, 0, sizeof(msg));
	thdr = (struct transsip_hdr *) msg;
	thdr->est = 1;

	ret = sendto(*csock, msg, sizeof(*thdr), 0, &ecurr.addr,
		     ecurr.addrlen);
	if (ret <= 0) {
		whine("Cannot send ring probe to server!\n");
		goto out_err;
	}

	fds[0].fd = *csock;
	fds[0].events = POLLIN;
	fds[1].fd = usocki;
	fds[1].events = POLLIN;

	while (!quit && tries++ < 100) {
		poll(fds, array_size(fds), 1500);

		for (i = 0; i < array_size(fds); ++i) {
			if ((fds[i].revents & POLLERR) == POLLERR) {
				printf("Destination unreachable?\n");
				goto out_err;
			}
			if ((fds[i].revents & POLLIN) != POLLIN)
				continue;

			if (fds[i].fd == usocki) {
				ret = read(usocki, &cpkt, sizeof(cpkt));
				if (ret <= 0) {
					whine("Read error from cli!\n");
					continue;
				}
				if (cpkt.fin) {
					memset(&msg, 0, sizeof(msg));
					thdr = (struct transsip_hdr *) msg;
					thdr->bsy = 1;
					thdr->fin = 1;

					sendto(*csock, msg, sizeof(*thdr), 0,
					       &ecurr.addr, ecurr.addrlen);

					whine("You aborted call!\n");
					goto out_err;
				}
			}

			if (fds[i].fd == *csock) {
				memset(msg, 0, sizeof(msg));
				raddrlen = sizeof(raddr);
				ret = recvfrom(*csock, msg, sizeof(msg), 0,
					       &raddr, &raddrlen);
				if (ret <= 0)
					continue;

				if (raddrlen != ecurr.addrlen)
					continue;
				if (memcmp(&raddr, &ecurr.addr, raddrlen))
					continue;

				thdr = (struct transsip_hdr *) msg;
				if (thdr->est == 1 && thdr->psh == 1) {
					ecurr.active = 1;
					whine("Call established!\n");
					return ENGINE_STATE_SPEAKING;
				}
				if (thdr->bsy == 1 || thdr->fin == 1) {
					whine("Remote end busy!\n");
					engine_play_busy(dev);
					engine_play_busy(dev);
					goto out_err;
				}
			}
		}

		engine_play_dial(dev);
	}

out_err:
	close(*csock);
	*csock = 0;
 	return ENGINE_STATE_IDLE;
}

static enum engine_state_num engine_do_callin(int ssock, int *csock, int usocki,
					      int usocko, struct alsa_dev *dev)
{
	int i;
	ssize_t ret;
	char msg[MAX_MSG];
	struct sockaddr raddr;
	socklen_t raddrlen;
	struct transsip_hdr *thdr;
	char hbuff[256], sbuff[256];
	struct pollfd fds[2];
	struct cli_pkt cpkt;

	assert(ecurr.active == 0);

	memset(&msg, 0, sizeof(msg));
	raddrlen = sizeof(raddr);
	ret = recvfrom(ssock, msg, sizeof(msg), 0, &raddr, &raddrlen);
	if (ret <= 0) {
		whine("Receive error %s!\n", strerror(errno));
		return ENGINE_STATE_IDLE;
	}

	thdr = (struct transsip_hdr *) msg;
	if (thdr->est != 1)
		return ENGINE_STATE_IDLE;

	memcpy(&ecurr.addr, &raddr, raddrlen);
	ecurr.addrlen = raddrlen;
	ecurr.sock = ssock;
	ecurr.active = 0;

	memset(hbuff, 0, sizeof(hbuff));
	memset(sbuff, 0, sizeof(sbuff));
	getnameinfo((struct sockaddr *) &raddr, raddrlen, hbuff, sizeof(hbuff),
		    sbuff, sizeof(sbuff), NI_NUMERICHOST | NI_NUMERICSERV);

	printf("New incoming connection from %s:%s!\n", hbuff, sbuff);
	printf("Answer it with: take\n");
	printf("Reject it with: hangup\n");
	fflush(stdout);

	fds[0].fd = ssock;
	fds[0].events = POLLIN;
	fds[1].fd = usocki;
	fds[1].events = POLLIN;

	while (likely(!quit)) {
		poll(fds, array_size(fds), 1500);

		for (i = 0; i < array_size(fds); ++i) {
			if ((fds[i].revents & POLLIN) != POLLIN)
				continue;

			if (fds[i].fd == ssock) {
				memset(msg, 0, sizeof(msg));
				raddrlen = sizeof(raddr);
				ret = recvfrom(ssock, msg, sizeof(msg), 0,
					       &raddr, &raddrlen);
				if (ret <= 0)
					continue;
				if (raddrlen != ecurr.addrlen)
					continue;
				if (memcmp(&raddr, &ecurr.addr, raddrlen))
					continue;

				if (thdr->fin == 1 || thdr->bsy == 1) {
					whine("Remote end hung up!\n");
					engine_play_busy(dev);
					engine_play_busy(dev);
					goto out_err;
				}

			}

			if (fds[i].fd == usocki) {
				ret = read(usocki, &cpkt, sizeof(cpkt));
				if (ret <= 0) {
					whine("Error reading from cli!\n");
					continue;
				}
				if (cpkt.fin) {
					memset(&msg, 0, sizeof(msg));
					thdr = (struct transsip_hdr *) msg;
					thdr->bsy = 1;
					thdr->fin = 1;

					sendto(ssock, msg, sizeof(*thdr), 0,
					       &ecurr.addr, ecurr.addrlen);

					whine("You aborted call!\n");
					goto out_err;
				}
				if (cpkt.take) {
					memset(&msg, 0, sizeof(msg));
					thdr = (struct transsip_hdr *) msg;
					thdr->est = 1;
					thdr->psh = 1;

					ret = sendto(ssock, msg, sizeof(*thdr), 0,
						     &ecurr.addr, ecurr.addrlen);
					if (ret <= 0) {
						whine("Error sending ack!\n");
						goto out_err;
					}

					ecurr.active = 1;
					whine("Call established!\n");
					return ENGINE_STATE_SPEAKING;
				}
			}
		}

		engine_play_ring(dev);
	}

out_err:
	return ENGINE_STATE_IDLE;
}

static enum engine_state_num engine_do_speaking(int ssock, int *csock,
						int usocki, int usocko,
						struct alsa_dev *dev)
{
	ssize_t ret;
	int recv_started = 0, nfds = 0, tmp, i;
	struct pollfd *pfds = NULL;
	char msg[MAX_MSG];
	uint32_t send_seq = 0;
	CELTMode *mode;
	CELTEncoder *encoder;
	CELTDecoder *decoder;
	JitterBuffer *jitter;
#ifdef HAS_SPEEX_AEC
	SpeexPreprocessState *preprocess;
	SpeexEchoState *echo_state;
	int one;
#endif
	struct sockaddr raddr;
	struct transsip_hdr *thdr;
	socklen_t raddrlen;
	struct cli_pkt cpkt;

	assert(ecurr.active == 1);

	mode = celt_mode_create(SAMPLING_RATE, FRAME_SIZE, NULL);
	encoder = celt_encoder_create(mode, 1, NULL);
	decoder = celt_decoder_create(mode, 1, NULL);

	jitter = jitter_buffer_init(FRAME_SIZE);
	tmp = FRAME_SIZE;
	jitter_buffer_ctl(jitter, JITTER_BUFFER_SET_MARGIN, &tmp);

#ifdef HAS_SPEEX_AEC
	echo_state = speex_echo_state_init(FRAME_SIZE, 10 * FRAME_SIZE);
	tmp = SAMPLING_RATE;
	speex_echo_ctl(echo_state, SPEEX_ECHO_SET_SAMPLING_RATE, &tmp);

	one = 1;
	preprocess = speex_preprocess_state_init(FRAME_SIZE, SAMPLING_RATE);
	speex_preprocess_ctl(preprocess, SPEEX_PREPROCESS_SET_DENOISE, &one);
	speex_preprocess_ctl(preprocess, SPEEX_PREPROCESS_SET_AGC, &one);
	speex_preprocess_ctl(preprocess, SPEEX_PREPROCESS_SET_DEREVERB, &one);
	speex_preprocess_ctl(preprocess, SPEEX_PREPROCESS_SET_ECHO_STATE,
			     echo_state);
#endif

	nfds = alsa_nfds(dev);
	pfds = xmalloc(sizeof(*pfds) * (nfds + 2));

	alsa_getfds(dev, pfds, nfds);

	pfds[nfds].fd = ecurr.sock;
	pfds[nfds].events = POLLIN;
	pfds[nfds + 1].fd = usocki;
	pfds[nfds + 1].events = POLLIN;

	alsa_start(dev);

	while (likely(!quit)) {
		poll(pfds, nfds + 2, -1);

		if (pfds[nfds + 1].revents & POLLIN) {
			ret = read(usocki, &cpkt, sizeof(cpkt));
			if (ret <= 0) {
				whine("Read error from cli!\n");
				continue;
			}
			if (cpkt.fin) {
				whine("You aborted call!\n");
				goto out_err;
			}
		}

		if (pfds[nfds].revents & POLLIN) {
			JitterBufferPacket packet;

			memset(msg, 0, sizeof(msg));
			raddrlen = sizeof(raddr);
			ret = recvfrom(ecurr.sock, msg, sizeof(msg), 0,
				       &raddr, &raddrlen);
			if (unlikely(ret <= 0))
				continue;

			if (raddrlen != ecurr.addrlen ||
			    memcmp(&raddr, &ecurr.addr, raddrlen)) {
				memset(msg, 0, sizeof(msg));

				thdr = (struct transsip_hdr *) msg;
				thdr->bsy = 1;
				thdr->fin = 1;

				sendto(ecurr.sock, msg, sizeof(*thdr), 0,
				       &raddr, raddrlen);

				goto out_alsa;
			}

			thdr = (struct transsip_hdr *) msg;
			if (thdr->fin == 1) {
				whine("Remote end hung up!\n");
				goto out_err;
			}

			packet.data = msg + sizeof(*thdr);
			packet.len = ret - sizeof(*thdr);
			packet.timestamp = ntohl(thdr->seq);
			packet.span = FRAME_SIZE;
			packet.sequence = 0;

			jitter_buffer_put(jitter, &packet);
			recv_started = 1;
		}
out_alsa:
		if (alsa_play_ready(dev, pfds, nfds)) {
			short pcm[FRAME_SIZE];

			if (recv_started) {
				JitterBufferPacket packet;

				memset(msg, 0, sizeof(msg));
				packet.data = msg;
				packet.len  = MAX_MSG;

				jitter_buffer_tick(jitter);
				jitter_buffer_get(jitter, &packet,
						  FRAME_SIZE, NULL);
				if (packet.len == 0)
					packet.data = NULL;

				celt_decode(decoder, (const unsigned char *)
					    packet.data, packet.len, pcm);
			} else {
				for (i = 0; i < FRAME_SIZE; ++i)
					pcm[i] = 0;
			}
#ifdef HAS_SPEEX_AEC
			if (alsa_write(dev, pcm, FRAME_SIZE))
				speex_echo_state_reset(echo_state);
			speex_echo_playback(echo_state, pcm);
#else
			alsa_write(dev, pcm, FRAME_SIZE);
#endif
		}

		if (alsa_cap_ready(dev, pfds, nfds)) {
			short pcm[FRAME_SIZE];

			alsa_read(dev, pcm, FRAME_SIZE);
#ifdef HAS_SPEEX_AEC
			short pcm2[FRAME_SIZE];
			speex_echo_capture(echo_state, pcm, pcm2);
			for (i = 0; i < FRAME_SIZE; ++i)
				pcm[i] = pcm2[i];

			speex_preprocess_run(preprocess, pcm);
#endif
			celt_encode(encoder, pcm, NULL, (unsigned char *)
				    (msg + sizeof(*thdr)), PACKETSIZE);

			memset(msg, 0, sizeof(msg));
			thdr = (struct transsip_hdr *) msg;
			thdr->psh = 1;
			thdr->est = 1;
			thdr->seq = htonl(send_seq);
			send_seq += FRAME_SIZE;

			ret = sendto(ecurr.sock, msg,
				     PACKETSIZE + sizeof(*thdr), 0,
				     &ecurr.addr, ecurr.addrlen);
			if (ret <= 0) {
				whine("Send datagram failed!\n");
				goto out_err;
			}
		}
	}

out_err:
	alsa_stop(dev);

	memset(msg, 0, sizeof(msg));
	thdr = (struct transsip_hdr *) msg;
	thdr->fin = 1;

	sendto(ecurr.sock, msg, sizeof(*thdr), 0, &ecurr.addr,
	       ecurr.addrlen);

	if (ecurr.sock == *csock) {
		close(*csock);
		*csock = 0;
	}

	celt_encoder_destroy(encoder);
	celt_decoder_destroy(decoder);
	celt_mode_destroy(mode);
#ifdef HAS_SPEEX_AEC
	speex_preprocess_state_destroy(preprocess);
	jitter_buffer_destroy(jitter);
#endif
	xfree(pfds);

	ecurr.active = 0;
	return ENGINE_STATE_IDLE;
}

static inline void engine_drop_from_queue(int sock)
{
	char msg[MAX_MSG];
	recv(sock, msg, sizeof(msg), 0);
}

static enum engine_state_num engine_do_idle(int ssock, int *csock, int usocki,
					    int usocko, struct alsa_dev *dev)
{
	int i;
	ssize_t ret;
	struct pollfd fds[2];
	char msg[MAX_MSG];
	struct transsip_hdr *thdr;
	struct cli_pkt cpkt;

	assert(ecurr.active == 0);

	fds[0].fd = ssock;
	fds[0].events = POLLIN;
	fds[1].fd = usocki;
	fds[1].events = POLLIN;

	while (likely(!quit)) {
		memset(msg, 0, sizeof(msg));

		poll(fds, array_size(fds), 1000);

		for (i = 0; i < array_size(fds); ++i) {
			if ((fds[i].revents & POLLIN) != POLLIN)
				continue;

			if (fds[i].fd == ssock) {
				ret = recv(ssock, msg, sizeof(msg), MSG_PEEK);
				if (ret < 0)
					continue;
				if (ret < sizeof(struct transsip_hdr))
					engine_drop_from_queue(ssock);

				thdr = (struct transsip_hdr *) msg;
				if (thdr->est == 1 && thdr->psh == 0)
					return ENGINE_STATE_CALLIN;
				else
					engine_drop_from_queue(ssock);
			}

			if (fds[i].fd == usocki) {
				ret = read(usocki, &cpkt, sizeof(cpkt));
				if (ret <= 0) {
					whine("Error reading from cli!\n");
					continue;
				}
				if (cpkt.ring)
					return ENGINE_STATE_CALLOUT;
			}
		}
	}

	return ENGINE_STATE_IDLE;
}

struct engine_state state_machine[__ENGINE_STATE_MAX] __read_mostly = {
	STATE_MAP_SET(ENGINE_STATE_IDLE, engine_do_idle),
	STATE_MAP_SET(ENGINE_STATE_CALLOUT, engine_do_callout),
	STATE_MAP_SET(ENGINE_STATE_CALLIN, engine_do_callin),
	STATE_MAP_SET(ENGINE_STATE_SPEAKING, engine_do_speaking),
};

void *engine_main(void *arg)
{
	int ssock = -1, csock, ret, mtu, usocki, usocko;
	enum engine_state_num state;
	struct addrinfo hints, *ahead, *ai;
	struct alsa_dev *dev = NULL;
	struct pipepair *pp = arg;

	init_call_notifier();

	usocki = pp->i;
	usocko = pp->o;

	while (!stun_done)
		sleep(0);

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	hints.ai_flags = AI_PASSIVE;

	ret = getaddrinfo(NULL, port, &hints, &ahead);
	if (ret < 0)
		panic("Cannot get address info!\n");

	for (ai = ahead; ai != NULL && ssock < 0; ai = ai->ai_next) {
		ssock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if (ssock < 0)
			continue;
		if (ai->ai_family == AF_INET6) {
			int one = 1;
#ifdef IPV6_V6ONLY
			ret = setsockopt(ssock, IPPROTO_IPV6, IPV6_V6ONLY,
					 &one, sizeof(one));
			if (ret < 0) {
				close(ssock);
				ssock = -1;
				continue;
			}
#else
			close(ssock);
			ssock = -1;
			continue;
#endif /* IPV6_V6ONLY */
		}

		mtu = IP_PMTUDISC_DONT;
		setsockopt(ssock, SOL_IP, IP_MTU_DISCOVER, &mtu, sizeof(mtu));

		ret = bind(ssock, ai->ai_addr, ai->ai_addrlen);
		if (ret < 0) {
			close(ssock);
			ssock = -1;
			continue;
		}
	}

	freeaddrinfo(ahead);
	if (ssock < 0)
		panic("Cannot open socket!\n");

	dev = alsa_open(alsadev, SAMPLING_RATE, 1, FRAME_SIZE);
	if (!dev)
		panic("Cannot open ALSA device %s!\n", alsadev);

	state = ENGINE_STATE_IDLE;
	while (likely(!quit)) {
		int arg = state;
		call_notifier_exec(CALL_STATE_MACHINE_CHANGED, &arg);
		state = state_machine[state].process(ssock, &csock,
						     usocki, usocko,
						     dev);
	}

	alsa_close(dev);
	close(ssock);

	pthread_exit(0);
}
