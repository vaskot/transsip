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
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <celt/celt.h>
#include <sys/ptrace.h>
#include <speex/speex_jitter.h>
#include <speex/speex_echo.h>
#include <sched.h>

#include "conf.h"
#include "curve.h"
#include "compiler.h"
#include "die.h"
#include "alsa.h"
#include "xmalloc.h"
#include "xutils.h"
#include "crypto_verify_32.h"
#include "crypto_box_curve25519xsalsa20poly1305.h"
#include "crypto_scalarmult_curve25519.h"
#include "crypto_auth_hmacsha512256.h"

noinline void *memset(void *__s, int __c, size_t __n);
extern void enter_shell_loop(void);
int show_key_export(char *home);

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

static void check_file_or_die(char *home, char *file, int maybeempty)
{
	char path[PATH_MAX];
	struct stat st;
	memset(path, 0, sizeof(path));
	slprintf(path, sizeof(path), "%s/%s", home, file);
	if (stat(path, &st))
		panic("No such file %s! Type --keygen to generate initial config!\n",
		      path);
	if (!S_ISREG(st.st_mode))
		panic("%s is not a regular file!\n", path);
	if ((st.st_mode & ~S_IFREG) != (S_IRUSR | S_IWUSR))
		panic("You have set too many permissions on %s (%o)!\n",
		      path, st.st_mode);
	if (maybeempty == 0 && st.st_size == 0)
		panic("%s is empty!\n", path);
}

static void check_config_exists_or_die(char *home)
{
	if (!home)
		panic("No home dir specified!\n");
	check_file_or_die(home, FILE_CONTACTS, 1);
	check_file_or_die(home, FILE_SETTINGS, 0);
	check_file_or_die(home, FILE_PRIVKEY, 0);
	check_file_or_die(home, FILE_PUBKEY, 0);
	check_file_or_die(home, FILE_USERNAME, 0);
}

static char *fetch_home_dir(void)
{
	char *home = getenv("HOME");
	if (!home)
		panic("No HOME defined!\n");
	return home;
}

static void write_username(char *home)
{
	int fd, ret;
	char path[PATH_MAX], *eof;
	char user[512];
	memset(path, 0, sizeof(path));
	slprintf(path, sizeof(path), "%s/%s", home, FILE_USERNAME);
	printf("Username: [%s] ", getenv("USER"));
	fflush(stdout);
	memset(user, 0, sizeof(user));
	eof = fgets(user, sizeof(user), stdin);
	user[sizeof(user) - 1] = 0;
	user[strlen(user) - 1] = 0; /* omit last \n */
	if (strlen(user) == 0)
		strlcpy(user, getenv("USER"), sizeof(user));
	fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if (fd < 0)
		panic("Cannot open your username file!\n");
	ret = write(fd, user, strlen(user));
	if (ret != strlen(user))
		panic("Could not write username!\n");
	close(fd);
	info("Username written to %s!\n", path);
}

static void create_transsipdir(char *home)
{
	int ret, fd;
	char path[PATH_MAX];
	memset(path, 0, sizeof(path));
	slprintf(path, sizeof(path), "%s/%s", home, ".transsip/");
	errno = 0;
	ret = mkdir(path, S_IRWXU);
	if (ret < 0 && errno != EEXIST)
		panic("Cannot create transsip dir!\n");
	info("transsip directory %s created!\n", path);
	/* We also create empty files for contacts and settings! */
	memset(path, 0, sizeof(path));
	slprintf(path, sizeof(path), "%s/%s", home, FILE_CONTACTS);
	fd = open(path, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
	if (fd < 0)
		panic("Cannot open contacts file!\n");
	close(fd);
	info("Empty contacts file written to %s!\n", path);
	memset(path, 0, sizeof(path));
	slprintf(path, sizeof(path), "%s/%s", home, FILE_SETTINGS);
	fd = open(path, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
	if (fd < 0)
		panic("Cannot open settings file!\n");
	close(fd);
	info("Empty settings file written to %s!\n", path);
}

static void create_keypair(char *home)
{
	int fd;
	ssize_t ret;
	unsigned char publickey[crypto_box_curve25519xsalsa20poly1305_PUBLICKEYBYTES] = { 0 };
	unsigned char secretkey[crypto_box_curve25519xsalsa20poly1305_SECRETKEYBYTES] = { 0 };
	char path[PATH_MAX];
	const char * errstr = NULL;
	int err = 0;
	info("Reading from %s (this may take a while) ...\n", ENTROPY_SOURCE);
	fd = open_or_die(ENTROPY_SOURCE, O_RDONLY);
	ret = read_exact(fd, secretkey, sizeof(secretkey), 0);
	if (ret != sizeof(secretkey)) {
		err = EIO;
		errstr = "Cannot read from "ENTROPY_SOURCE"!\n";
		goto out;
	}
	close(fd);
	crypto_scalarmult_curve25519_base(publickey, secretkey);
	memset(path, 0, sizeof(path));
	slprintf(path, sizeof(path), "%s/%s", home, FILE_PUBKEY);
	fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if (fd < 0) {
		err = EIO;
		errstr = "Cannot open pubkey file!\n";
		goto out;
	}
	ret = write(fd, publickey, sizeof(publickey));
	if (ret != sizeof(publickey)) {
		err = EIO;
		errstr = "Cannot write public key!\n";
		goto out;
	}
	close(fd);
	info("Public key written to %s!\n", path);
	memset(path, 0, sizeof(path));
	slprintf(path, sizeof(path), "%s/%s", home, FILE_PRIVKEY);
	fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if (fd < 0) {
		err = EIO;
		errstr = "Cannot open privkey file!\n";
		goto out;
	}
	ret = write(fd, secretkey, sizeof(secretkey));
	if (ret != sizeof(secretkey)) {
		err = EIO;
		errstr = "Cannot write private key!\n";
		goto out;
	}
out:
	close(fd);
	memset(publickey, 0, sizeof(publickey));
	memset(secretkey, 0, sizeof(secretkey));
	if (err)
		panic("%s: %s", errstr, strerror(errno));
	else
		info("Private key written to %s!\n", path);
}

static void check_config_keypair_or_die(char *home)
{
	int fd;
	ssize_t ret;
	int err;
	const char * errstr = NULL;
	unsigned char publickey[crypto_box_curve25519xsalsa20poly1305_PUBLICKEYBYTES];
	unsigned char publicres[crypto_box_curve25519xsalsa20poly1305_PUBLICKEYBYTES];
	unsigned char secretkey[crypto_box_curve25519xsalsa20poly1305_SECRETKEYBYTES];
	char path[PATH_MAX];
	memset(path, 0, sizeof(path));
	slprintf(path, sizeof(path), "%s/%s", home, FILE_PRIVKEY);
	fd = open(path, O_RDONLY);
	if (fd < 0) {
		err = EIO;
		errstr = "Cannot open privkey file!\n";
		goto out;
	}
	ret = read(fd, secretkey, sizeof(secretkey));
	if (ret != sizeof(secretkey)) {
		err = EIO;
		errstr = "Cannot read private key!\n";
		goto out;
	}
	close(fd);
	memset(path, 0, sizeof(path));
	slprintf(path, sizeof(path), "%s/%s", home, FILE_PUBKEY);
	fd = open(path, O_RDONLY);
	if (fd < 0) {
		err = EIO;
		errstr = "Cannot open pubkey file!\n";
		goto out;
	}
	ret = read(fd, publickey, sizeof(publickey));
	if (ret != sizeof(publickey)) {
		err = EIO;
		errstr = "Cannot read public key!\n";
		goto out;
	}
	crypto_scalarmult_curve25519_base(publicres, secretkey);
	err = crypto_verify_32(publicres, publickey);
	if (err) {
		err = EINVAL;
		errstr = "WARNING: your keypair is corrupted!!! You need to "
			 "generate new keys!!!\n";
		goto out;
	}
out:
	close(fd);
	memset(publickey, 0, sizeof(publickey));
	memset(publicres, 0, sizeof(publicres));
	memset(secretkey, 0, sizeof(secretkey));
	if (err)
		panic("%s: %s\n", errstr, strerror(errno));
}

static int keygen(char *home)
{
	create_transsipdir(home);
	write_username(home);
	create_keypair(home);
	check_config_keypair_or_die(home);
	printf("\nNow edit your .transsip/settings file before starting i.e.:\n");
	printf(" port <your-transsip-port>\n");
	printf(" stun stunserver.org\n");
	printf(" alsa plughw:0,0\n");
	return 0;
}

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
	char *home = fetch_home_dir();
	if (getuid() != geteuid())
		seteuid(getuid());
	if (getenv("LD_PRELOAD"))
		panic("transsip cannot be preloaded!\n");
	if (ptrace(PTRACE_TRACEME, 0, 1, 0) < 0)
		panic("transsip cannot be ptraced!\n");
	curve25519_selftest();
	if (argc == 2 && !strncmp("--keygen", argv[1], strlen("--keygen"))) {
		keygen(home);
		show_key_export(home);
		return 0;
	}
	check_config_exists_or_die(home);
	check_config_keypair_or_die(home);
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

int show_key_export(char *home)
{
	int fd, i;
	ssize_t ret;
	char path[PATH_MAX], tmp[64];
	check_config_exists_or_die(home);
	check_config_keypair_or_die(home);
	printf("Your public information:\n ");
	fflush(stdout);
	memset(path, 0, sizeof(path));
	slprintf(path, sizeof(path), "%s/%s", home, FILE_USERNAME);
	fd = open_or_die(path, O_RDONLY);
	while ((ret = read(fd, tmp, sizeof(tmp))) > 0) {
		ret = write(STDOUT_FILENO, tmp, ret);
	}
	close(fd);
	printf(" ");
	memset(path, 0, sizeof(path));
	slprintf(path, sizeof(path), "%s/%s", home, FILE_PUBKEY);
	fd = open_or_die(path, O_RDONLY);
	ret = read(fd, tmp, sizeof(tmp));
	if (ret != crypto_box_curve25519xsalsa20poly1305_PUBLICKEYBYTES)
		panic("Cannot read public key!\n");
	for (i = 0; i < ret; ++i)
		if (i == ret - 1)
			printf("%02x\n\n", (unsigned char) tmp[i]);
		else
			printf("%02x:", (unsigned char) tmp[i]);
	close(fd);
	fflush(stdout);
	return 0;
}

