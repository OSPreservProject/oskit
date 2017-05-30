#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include "bootwhat.h"

/*
 * For now, hardwired.
 */
#define NETBOOT		"/tftpboot/netboot"

main()
{
	int			sock, length, data, i, mlen;
	struct sockaddr_in	name, client;
	boot_info_t		boot_info;
	boot_what_t	       *boot_whatp = (boot_what_t *) &boot_info.data;

	/* Create socket from which to read. */
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		perror("opening datagram socket");
		exit(1);
	}
	
	/* Create name. */
	name.sin_family = AF_INET;
	name.sin_addr.s_addr = INADDR_ANY;
	name.sin_port = htons(BOOTWHAT_DSTPORT);
	if (bind(sock, (struct sockaddr *) &name, sizeof(name))) {
		perror("binding datagram socket");
		exit(1);
	}
	/* Find assigned port value and print it out. */
	length = sizeof(name);
	if (getsockname(sock, (struct sockaddr *) &name, &length)) {
		perror("getting socket name");
		exit(1);
	}
	printf("Socket has port #%d\n", ntohs(name.sin_port));

	while (1) {
		if ((mlen = recvfrom(sock, &boot_info, sizeof(boot_info),
				     0, (struct sockaddr *)&client, &length))
		    < 0) {
			perror("receiving datagram packet");
			exit(1);
		}

		printf("Datagram of %d bytes received from %s\n",
			mlen, inet_ntoa(client.sin_addr));

		if (boot_info.opcode != BIOPCODE_BOOTWHAT_REQUEST) {
			printf("Throwing away invalid packet\n");
			continue;
		}
		boot_info.opcode     = BIOPCODE_BOOTWHAT_REPLY;
		boot_info.status     = BISTAT_SUCCESS;
		boot_info.cmdline[0] = 0;
#if 0
		boot_whatp->type = BIBOOTWHAT_TYPE_MB;
		boot_whatp->what.mb.tftp_ip.s_addr = 0;
		strcpy(boot_whatp->what.mb.filename, NETBOOT);
#else
		boot_whatp->type = BIBOOTWHAT_TYPE_SYSID;
		boot_whatp->what.sysid = 131; /* LINUXOS */
#endif
		client.sin_family = AF_INET;
		client.sin_port = htons(BOOTWHAT_SRCPORT);
		if (sendto(sock, (char *)&boot_info, sizeof(boot_info), 0,
			(struct sockaddr *)&client, sizeof(client)) < 0)
			perror("sendto");
	}
	close(sock);
}

