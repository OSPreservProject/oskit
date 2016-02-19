/*
 * Copyright (c) 1995-2001 University of Utah and the Flux Group.
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

#include <ctype.h>		/* isspace */
#include <stdlib.h>		/* malloc */
#include <string.h>		/* strcpy */
#include <stdio.h>		/* printf */
#include <assert.h>		/* assert */
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>		/* ntohl */
#include <malloc.h>		/* malloc_lmm */
#include <netdb.h>

#include <oskit/dev/dev.h>
#include <oskit/clientos.h>
#include <oskit/startup.h>
#ifdef  PTHREADS
#include <oskit/threads/pthread.h>
#endif

#include "driver.h"
#include "timer.h"
#include "image.h"
#include "misc.h"

#ifdef UTAHTESTBED
static void bootinfo_ack(void);
#endif

#define CMD_LINE_LEN 512

extern char		version[], build_info[];

/* Forward decls */
static void		print_help(void);
static int		runcommand(char *filename, char *diskname);

int
main(int argc, char *argv[])
{
	char		buf[BUFSIZ], input[BUFSIZ], prev_cmd[BUFSIZ];

#ifdef  PTHREADS
	oskit_clientos_init_pthreads();
	start_clock();
	start_pthreads();
#else
	oskit_clientos_init();
	start_clock();
#endif
	printf("\n%s\n", version);
	printf("%s\n", build_info);

	osenv_process_lock();
	start_net_devices();
	start_blk_devices();
	osenv_process_unlock();
	start_fs_bmod();
	heartbeat_init();
	timer_init();
	net_init();

	/* Skip kernel name */
	argc--;
	argv++;

	if (argc) {
		/*
		 * Allow for several netdisk commands to specified.
		 */
		while (argc) {
			if (argc < 2) {
				print_help();
				exit(1);
			}
			if (runcommand(argv[0], argv[1]))
				goto bad;

			argv += 2;
			argc -= 2;
		}
		goto done;
	}
	
	/*
	 * Interactive mode
	 */
	strcpy(prev_cmd, "");
	while (1) {
		char	*path, *disk;
			
		showdisks();
		printf("NetDisk> ");

		fgets(input, BUFSIZ, stdin);
		input[strlen(input) - 1] = '\0';	/* chop \n */

		if (strcmp(input, "help") == 0 || strcmp(input, "?") == 0) {
			print_help();
			continue;
		}

		if (strcmp(input, "quit") == 0 || strcmp(input, "exit") == 0)
			break;

		if (input[0] == '\0')
			continue;
		
		if (strcmp(input, "!!") == 0) {
			/* get the previous command */
			strcpy(buf, prev_cmd);
		} else {
			/* save the command for next time round */
			strcpy(prev_cmd, input);
			strcpy(buf, input);
		}

		/*
		 * Input line consists of a path and a disk spec only.
		 */
		path = disk = buf;
		while (*disk && !isspace(*disk))
			disk++;

		if (!*disk) {
			printf("Bad command line, type \"help\" for help.\n");
			continue;
		}
		*disk++ = NULL;
		while (isspace(*disk))
			disk++;

		if (runcommand(path, disk))
			continue;
	}

 done:
#ifdef UTAHTESTBED
	bootinfo_ack();
#endif
 bad:
	net_shutdown();
	timer_shutdown();
	exit(0);
}

static int
runcommand(char *imagespec, char *diskname)
{
	struct in_addr	ip;		/* ip of server host, in host order */
	char		*dirname;	/* dir to mount */
	char		*filename;	/* file within dir */

	if (! parse_filespec(imagespec, &ip.s_addr, &dirname, &filename)) {
		printf("usage: <IPaddr|host>:<path> <disk>[:part]\n");
		return 1;
	}

	netprintf("Root server: %I, dir: %s, file: %s, disk/part %s\r\n",
		  ip.s_addr, dirname, filename, diskname);

	if (writeimage(ip, dirname, filename, diskname))
		return 1;

	return 0;
}

static void
print_help(void)
{
	printf("\n");
	printf("%s\n", version);
	printf("%s\n", build_info);
	printf("\n");
	printf("To copy over the disk image, tell me the disk file name and\n"
	       "the partition to which it should copied, in the form\n"
	       "\t<IPaddr|host>:<path> <partition>[:partition]\n"
	       "For example:\n"
	       "\t155.22.33.44:/home/foo/diskimage wd1"
	       "or\n"
	       "\tmyhost:/home/foo/diskimage wd1:b\n"
	       "The directory must be an NFS exported directory.\n"
	       "Hostnames are currently limited to \"marker\" and \"fast\".\n");
	printf("\n");
}

#if	defined(UTAHTESTBED)
#include <oskit/boot/bootwhat.h>
#include "socket.h"

/* XXX */
#define  BOOTSERVER	"boss.emulab.net"

static void
bootinfo_ack(void)
{
	int			i;
	boot_info_t		bootinfo;
	oskit_socket_t		*sock;
	oskit_size_t		got;
	struct oskit_sockaddr	sockaddr;
	struct sockaddr_in	*sin = (void *) &sockaddr;
	
	/*
	 * Create a socket to talk to the server. Use that for
	 * all RPC request traffic.
	 */
	if (comsocket(OSKIT_AF_INET, OSKIT_SOCK_DGRAM, IPPROTO_UDP, &sock)) {
		printf("bootinfo_ack: comsocket failed\n");
		return;
	}

	/* Bind our side */
	sin->sin_family      = OSKIT_AF_INET;
	sin->sin_addr.s_addr = htonl(INADDR_ANY);
	sin->sin_port        = htons(BOOTWHAT_SRCPORT);
	if (oskit_socket_bind(sock, &sockaddr, sizeof(sockaddr))) {
		oskit_socket_release(sock);
		printf("bootinfo_ack: oskit_socket_bind failed\n");
		return;
	}

	bootinfo.opcode = BIOPCODE_BOOTWHAT_ACK;
	bootinfo.status = BISTAT_SUCCESS;

	sin->sin_addr.s_addr = htonl(hostlookup(BOOTSERVER));
	if (sin->sin_addr.s_addr == INADDR_ANY) {
		printf("bootinfo_ack: Can not resolve %s\n", BOOTSERVER);
		oskit_socket_release(sock);
		return;
	}
	sin->sin_port = htons(BOOTWHAT_DSTPORT);

	for (i = 0; i < 3; i++)
		oskit_socket_sendto(sock, &bootinfo, sizeof(bootinfo), 0,
				    &sockaddr, sizeof(sockaddr), &got);

	oskit_socket_release(sock);
}
#endif
