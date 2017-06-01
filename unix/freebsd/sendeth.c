/*
 * Copyright (c) 1997, 2000 University of Utah and the Flux Group.
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
 * no, I haven't heard of libpcap....
 */
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/param.h>		/* for BSD */
#include <sys/socket.h>
#include <net/bpf.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <fcntl.h>
#include <oskit/dev/dev.h>

#include "native.h"
#include "native_errno.h"


/*
 * install a bpf excluding IEEE 802.3 packets
 */
static void 
exclude_ieee802_3(int dev)
{
	struct bpf_insn insns[] = {
		/* copy halfword at absolute offset 12 in accummulator */
		BPF_STMT(BPF_LD+BPF_H+BPF_ABS, 12),
		/* if greater or equal to 1500, advance 1, else 2 instr */
		BPF_JUMP(BPF_JMP+BPF_JGE+BPF_K, 1500, 0, 1),
		/* accept packet */
		BPF_STMT(BPF_RET+BPF_K, (u_int)-1),
		/* reject packet */
		BPF_STMT(BPF_RET+BPF_K, 0),
	};

	struct bpf_program pr = { sizeof(insns)/sizeof(insns[0]), insns } ;
	int    err;

	err = NATIVEOS(ioctl)(dev, BIOCSETF, &pr);
	if (err == -1)
		oskitunix_perror("BIOCSETF"), exit(-1);
}

int
oskitunix_open_eth(char *ifname, char *myaddr,
		   unsigned int *buflen, int no_ieee802_3)
{
	struct ifreq ifr;
	int dev = 0, rc, i;
	int on = 1;
	char *bp, *msg;
	char bpfstr[] = "/dev/bpfN";

	/*
	 * Find an available BPF device
	 */
	for (i = 0; i < 10; i++) {
		bpfstr[8] = i + '0';
		dev = NATIVEOS(open)(bpfstr, O_RDWR);
		if (dev >= 0)
			break;
	}
	if (i == 10) {
		osenv_log(OSENV_LOG_ERR, "No available /dev/bpf devices\n");
		return -1;
	}

	rc = NATIVEOS(ioctl)(dev, BIOCGBLEN, buflen);
	if (rc == -1) {
		msg = "BIOCGBLEN";
		goto failed;
	}
	/* printf("buflen set to %d\n", *buflen); */

	strcpy(ifr.ifr_name, ifname);
	rc = NATIVEOS(ioctl)(dev, BIOCSETIF, &ifr);
	if (rc == -1) {
		msg = "BIOCSETIF";
		goto failed;
	}

	/*
	 * BIOCIMMEDIATE (u_int)
	 * Enable  or  disable ``immediate mode'', based on the truth
	 * value of the argument.  When immediate mode  is  enabled,
	 * reads return immediately upon packet reception.  Otherwise,
	 * a read will  block until either the kernel buffer becomes
	 * full or a timeout occurs.  This  is  useful  for  programs
	 * like  rarpd(8c),  which must respond to messages in real
	 * time.  The default for  a  new  file  is off.
	 *
	 * I definitely need this because else bpf_wakeup wouldn't be called.
	 */
	rc = NATIVEOS(ioctl)(dev, BIOCIMMEDIATE, &on);
	if (rc == -1) {
		msg = "BIOCIMMEDIATE";
		goto failed;
	}

	if (no_ieee802_3)
		exclude_ieee802_3(dev);

	rc = NATIVEOS(ioctl)(dev, BIOCPROMISC, 0);
	if (rc == -1) {
		msg = "BIOCPROMISC";
		goto failed;
	}

	rc = NATIVEOS(ioctl)(dev, BIOCGETIF, &ifr);
	if (rc == -1) {
		msg = "BIOCGETIF";
		goto failed;
	}
	/* printf("connected to interface `%s'\n", ifr.ifr_name); */	

	if ((bp = (char *)getenv("ETHERADDR")) == NULL) {
		rc = NATIVEOS(ioctl)(dev, SIOCGIFADDR, &ifr);
		if (rc == -1) {
			msg = "SIOCGIFADDR";
			goto failed;
		}
		memcpy(myaddr, &ifr.ifr_ifru.ifru_addr.sa_data,
		       ETHER_ADDR_LEN);
	} else {
		int x0, x1, x2, x3, x4, x5;

		sscanf(bp, "%x:%x:%x:%x:%x:%x", &x0, &x1, &x2, &x3, &x4, &x5);

		myaddr[0] = x0;
		myaddr[1] = x1;
		myaddr[2] = x2;
		myaddr[3] = x3;
		myaddr[4] = x4;
		myaddr[5] = x5;
	}

	/*
	 * We have to do this ourselves, since oskitunix_set_async_fd
	 * is only a stub function in the non-threaded library. =(
	 */
	rc = NATIVEOS(fcntl)(dev, F_SETOWN, NATIVEOS(getpid)());
	if (rc == -1) {
		msg = "F_SETOWN";
		goto failed;
	}

	rc = NATIVEOS(fcntl)(dev, F_SETFL, O_ASYNC | O_NONBLOCK);
	if (rc == -1) {
		msg = "F_SETFL";
		goto failed;
	}

	return dev;

 failed:
	if (msg)
		oskitunix_perror(msg);
	NATIVEOS(close)(dev);
	return -1;
}

void
oskitunix_close_eth(int fd)
{
	NATIVEOS(close)(fd);
}
