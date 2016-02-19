/*
 * Copyright (c) 1997-1998 University of Utah and the Flux Group.
 * All rights reserved.
 * 
 * This file is part of the Flux OSKit.  The OSKit is free software, also known
 * as "open source;" you can redistribute it and/or modify it under the terms
 * of the GNU General Public License (GPL), version 2, as published by the Free
 * Software Foundation (FSF).  To explore alternate licensing terms, contact
 * the University of Utah at csl-dist@cs.utah.edu or +1-801-585-3271.
 * 
 * The OSKit is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GPL for more details.  You should have
 * received a copy of the GPL along with the OSKit; see the file COPYING.  If
 * not, write to the FSF, 59 Temple Place #330, Boston, MA 02111-1307, USA.
 */
/*
 * This demonstrates using the network stack with block device drivers,
 * as well as porting Unix-derived code to the OSKit.
 *
 * This program sends/receives a file over the net.
 * You run one in recv mode on one machine,
 * and one in send mode on another.
 *
 * This was originally written for Unix,
 * but was ported to the OSKit.
 * Under Unix it can use any file,
 * but under the OSKit it must use a block device (e.g., wd0).
 *
 * Usage: see usage() below.
 *
 * TODO:
 * Add partitioning support to the OSKit code to read/write
 *	a partition instead of the entire disk device.
 */

/* Under the OSKit, this must be a multiple of the sector size */
#define BUF_SIZE 16384		/* how many bytes per send/recv */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>

#ifdef OSKIT
#include <stdio.h>
#include <oskit/dev/dev.h>
#include <oskit/dev/ethernet.h>
#include <oskit/io/blkio.h>
#include <assert.h>
#include <oskit/c/sys/time.h>

extern struct timeval FDEV_FREEBSD_time;
#endif

char *server = NULL;
int port = 0;

char *filename = NULL;

void
usage(char *prog)
{
	/* Under the OSKit, the `file' currently must be a block device */
	printf("This copies a file from the sender to the receiver\n");
	printf("usage:\n");
	printf("%s send local_filename recv_address <portnum>\n\tor\n"
		"%s recv local_filename <portnum>\n", prog, prog);
	exit(1);
}

void
parse_command_line(int argc, char *argv[])
{
	if (argc < 3)
		usage(argv[0]);

	filename = argv[2];
	if (!strcmp("send", argv[1])) {
		server = argv[3];
		if (!server)
			usage(argv[0]);
		if (argv[4])
			sscanf(argv[4], "%d", &port);
	} else if (!strcmp("recv", argv[1])) {
		if (argv[3])
			sscanf(argv[3], "%d", &port);
	} else
		usage(argv[0]);
}

int
tcp_main()
{
	static int loops = 0;
	int sd;
#ifdef OSKIT
	oskit_blkio_t *io = NULL;
#else
	int fd;
#endif
	int rc;
	int count;
	static char buf[BUF_SIZE];	/* don't stack allocate large array */
	struct sockaddr_in addr;
	int addr_len = sizeof(addr);

	printf("%s %d\n", server, port);

	/* create the socket */
	sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sd < 0) {
		perror("socket"); exit(-3);
	}
	addr.sin_port = htons(port);
	
	/* is this the server side or send side? */
	if (server == NULL) { /* recv has NULL server */
		int nsd;

		/*
		 * This is the receive path
		 * This creates/overwrites the local file
		 */

		addr.sin_addr.s_addr = htonl(INADDR_ANY);

		rc = bind(sd, &addr, sizeof(addr));
		if (rc < 0) {
			perror("bind\n"); exit(-3);
		}
		/* if anonymous port, get actual port number */
		if (port == 0) {
			rc = getsockname(sd, &addr, &addr_len);
			port = ntohs(addr.sin_port);
		}
		printf("server (recv) on port %d\n", port);
		listen(sd, 1);	/* only deal with 1 client anyway... */

		/* use addr for both sd and nsd */
		addr_len = sizeof(addr);
		nsd = accept(sd, &addr, &addr_len);
		if (nsd < 0) {
			perror("accept"); exit(-3);
		}
		close(sd);

#ifdef OSKIT
		rc = oskit_linux_block_open(filename, OSKIT_DEV_OPEN_WRITE, &io);
		assert(rc == 0);
#else
		/* open the file for writing */
		fd = open(filename, O_WRONLY | O_CREAT, 0640);
		if (fd < 0) {
			perror("open"); exit(-3);
		}
#endif

		/* okay, here is where I service `requests' on nsd */
		count = BUF_SIZE;
		while (count > 0) {
			/* fill up buf if possible */
			count = recv(nsd, buf, BUF_SIZE, MSG_WAITALL);
			if (count < BUF_SIZE)
				printf("wrote %d bytes\n", count);
#ifdef OSKIT
			/* write to the block device */
			rc = oskit_blkio_write(io, buf, loops * BUF_SIZE,
				BUF_SIZE, &count);
			if (rc)
				printf("rc = %d\n", rc);
			assert(rc == NULL);
#else
			write(fd, buf, count);
#endif
			loops ++;
			if (!loops % 1024)
				printf("%d MB done\n",
					(loops / 1024) * (BUF_SIZE/1024));
		}
		close(nsd);
#ifdef OSKIT
		oskit_blkio_release(io);
#else
		close(fd);
#endif

	} else {

		/*
		 * This is the send path
		 * This reads the local file & sends it
		 */

		printf("client (send) connecting to port %d on %s\n", port,
			server);

		if (isdigit(server[0])) {
			addr.sin_addr.s_addr = inet_addr(server);
		} else {
			printf("deal with gethostbyname\n");
			exit(-2);
		}
		addr.sin_family = AF_INET;

		addr_len = sizeof(addr);
		rc = connect(sd, &addr, addr_len);
		if (rc < 0) {
			perror("connect"); exit(-3);
		}

		/* open the file for reading */
#ifdef OSKIT
		rc = oskit_linux_block_open(filename, OSKIT_DEV_OPEN_READ, &io);
		assert(rc == 0);
#else
		fd = open(filename, O_RDONLY, 0400);
		if (fd < 0) {
			perror("open"); exit(-3);
		}
#endif

		/* send loop */
		count = BUF_SIZE;
		while (count == BUF_SIZE) {
#ifdef OSKIT
			rc = oskit_blkio_read(io, buf, loops * BUF_SIZE,
				BUF_SIZE, &count);
			assert(rc == NULL);
#else
			count = read(fd, buf, BUF_SIZE);
#endif
			if (count < BUF_SIZE)
				printf("read %d bytes\n", count);
			rc = send(sd, buf, count, 0);
			loops++;
			if (rc < 0) {
				printf("send returned %d\n", rc);
#ifdef OSKIT
				break;
#endif
			}
		}
#ifdef OSKIT
		oskit_blkio_release(io);
#else
		close(fd);
#endif
		close(sd);
	}
	printf("%d loops\n", loops);
	printf("exiting normally\n");
}

int
main(int argc, char *argv[])
{
#ifdef OSKIT
	char **ap, *argv[10], *inputstring;
	int argc = 1;
	char IPADDR[20], NETMASK[20], GATEWAY[20];

	int err, ndev;
	oskit_etherdev_t **etherdev;
	oskit_socket_factory_t *fsc;
	struct oskit_freebsd_net_ether_if *eif;


	printf("Initializing devices...\n");
	oskit_dev_init();

	/* Init the drivers you want.  This does all the Linux ones */
	oskit_linux_init_devs();

	printf("Probing devices...\n");
	oskit_dev_probe();
	oskit_dump_devices();

	/* Find all the Ethernet devices. */
	ndev = osenv_device_lookup(&oskit_etherdev_iid, (void***)&etherdev);
	if (ndev <= 0)
		panic("no Ethernet adaptors found!");

	get_ipinfo(etherdev[0], IPADDR, GATEWAY, NETMASK, NULL);

	err = oskit_freebsd_net_init(&fsc);	/* initialize network code */
	assert(!err);

	fd_set_socketfactory(fsc);

	/* Configure the first device */
	err = oskit_freebsd_net_open_ether_if(etherdev[0], &eif);
	assert(!err);

	err = oskit_freebsd_net_ifconfig(eif, "eth0", IPADDR, NETMASK);
	assert(!err);
	err = oskit_freebsd_net_add_default_route(GATEWAY);
	if (err)
		printf("couldn't add default route (%d)\n", err);


	argv[0] = "oskit";
	inputstring = malloc(100);

	printf("Enter command line: ");
	gets(inputstring);
	printf("cmd line is \"%s\"\n", inputstring);
	for (ap = argv+1; (*ap = strsep(&inputstring, " \t")) != NULL; argc++)
		if (**ap != '\0')
			++ap;
#endif

	parse_command_line(argc, argv);

#ifdef OSKIT
	printf("It is %s", asctime(localtime(&FDEV_FREEBSD_time.tv_sec)));
#endif

	tcp_main();

#ifdef OSKIT
	printf("It is %s", asctime(localtime(&FDEV_FREEBSD_time.tv_sec)));
#endif

#if OSKIT
	/* close etherdev and release net_io devices */
	oskit_freebsd_net_close_ether_if(eif);
#endif /* OSKIT */
}

