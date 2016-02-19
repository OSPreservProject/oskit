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

#ifdef FILEOPS_TFTP
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <malloc.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/tftp.h>
#include <oskit/dev/dev.h>
#include <oskit/machine/multiboot.h>

#include "fileops.h"
#include "debug.h"
#include "misc.h"
#include "socket.h"

#define TFTP_PORT	69
#define TFTP_BUFSIZE	(SEGSIZE+4)
#define REQ_RETRIES	5

static struct in_addr srvaddr;
static unsigned short srvport;
static oskit_socket_t *srvsock;
static char reqbuf[TFTP_BUFSIZE], replybuf[TFTP_BUFSIZE];
static struct tftphdr *req;
static int reqsize;
static int currentblock, currentsize;
static void *currentdata;

static int getnextblock(void);
static int bsch_load(int blkno, oskit_addr_t reqoff, oskit_size_t reqsize,
		     void *data);
static void bsch_store(int blkno, oskit_addr_t reqoff, oskit_size_t reqsize,
		       void *data, int datasize);
static void bsch_flush(void);

/*
 * Send a read request for the specified file and wait for the first
 * block or a NAK.
 */
int
tftpopen(struct in_addr ip, char *dirname, char *filename)
{
	oskit_socket_t *sock;
	struct oskit_sockaddr sockaddr;
	struct sockaddr_in *sin = (void *)&sockaddr;
	oskit_size_t len, blen;
	int retval = -1;

	/*
	 * Create a socket to talk to the server.
	 */
	if (comsocket(OSKIT_AF_INET, OSKIT_SOCK_DGRAM, IPPROTO_UDP, &sock)) {
		printf("tftpopen: comsocket failed\n");
		return -1;
	}
	memset(sin, 0, sizeof(*sin));
	sin->sin_family = OSKIT_AF_INET;
	if (oskit_socket_bind(sock, &sockaddr, sizeof(sockaddr))) {
		printf("tftpopen: oskit_socket_bind failed\n");
		goto done;
	}

	/*
	 * Initialize the request buffer
	 * Note +1's which include nulls at end of file name and mode.
	 */
	req = (struct tftphdr *)reqbuf;
	req->th_opcode = htons(RRQ);
	blen = SEGSIZE - sizeof(req->th_opcode);
	snprintf(req->th_stuff, blen, "%s/%s", dirname, filename);
	len = strlen(req->th_stuff) + 1;
	if (blen - len < 6) {
		printf("tftpopen: filename too long\n");
		goto done;
	}
	strcpy(req->th_stuff+len, "octet");
	reqsize = sizeof(req->th_opcode) + len + 6;

	srvaddr.s_addr = htonl(ip.s_addr);
	srvport = htons(TFTP_PORT);
	srvsock = sock;

	/*
	 * Request the initial block.
	 * This will validate existance of the file.
	 */
	currentblock = 0;
	retval = getnextblock();
	if (retval) {
		printf("tftpopen: initial request failed\n");
		goto done;
	}

 done:
	if (retval) {
		/* XXX send a NAK? */
		oskit_socket_release(sock);
		srvsock = 0;
	}
	return retval;
}

/*
 * So we can do multiple loads.
 */
void
tftpclose(void)
{
	/*
	 * Close any old connection
	 * XXX send an error to the other side?
	 */
	if (srvsock) {
		oskit_socket_release(srvsock);
		srvsock = 0;
	}

	bsch_flush();
}

/*
 * Read a small block of data from the input file.
 */
int
tftpread(oskit_addr_t file_ofs, void *buf, oskit_size_t size,
	 oskit_size_t *out_actual)
{
	int n, bytes_read;
	char *bufp = buf;
	int block;
	int err = 0;

#if 0
	printf("Want foff=%d(0x%x) size=%d", file_ofs, file_ofs, size);
#endif
	bytes_read = 0;
	while (size > 0) {
		block = file_ofs / SEGSIZE + 1;
#if 0
		printf("\n read %d: block=%d, curblock=%d, cursize=%d\n",
		       bytes_read, block, currentblock, currentsize);
#endif
		n = bsch_load(block, file_ofs, size, bufp);
		if (n < 0) {
			assert(block >= currentblock);
			while (currentblock != block) {
				/* last block has been read */
				if (currentsize < SEGSIZE)
					goto done;
				
				/* read next block */
				err = getnextblock();
				if (err) {
					printf("TFTP: read error 0x%x\n", err);
					return 1;
				}
			}

			bsch_store(block, file_ofs, size,
				   currentdata, currentsize);

			n = size < currentsize ? size : currentsize;
			memcpy(bufp, currentdata, n);
		}

		file_ofs += n;
		bufp += n;
		size -= n;
		bytes_read += n;
	}

 done:
#if 0
	printf(" return %d bytes, sum=0x%x\n",
	       bytes_read, datasum(buf, bytes_read));
#endif
        *out_actual = bytes_read;
        return 0;
}

/*
 * Receive and ACK the next file block.
 * On success, updates currentblock/currentsize and returns zero.
 * On error, return an error code.
 */
static int
getnextblock(void)
{
	struct tftphdr *reply;
	struct oskit_sockaddr sockaddr;
	struct sockaddr_in *sin = (void *)&sockaddr;
	int retries, sendreq;
	oskit_size_t got, slen;
	int err, i;

	err = OSKIT_E_ABORT;
	sendreq = currentblock == 0 ? 1 : 0;
	for (retries = 0; retries < REQ_RETRIES; retries++) {
		/*
		 * (Re)send the request/ACK
		 */
		if (sendreq) {
			sin->sin_family = OSKIT_AF_INET;
			sin->sin_addr.s_addr = srvaddr.s_addr;
			sin->sin_port = srvport;
			err = oskit_socket_sendto(srvsock, req, reqsize, 0,
						  &sockaddr, sizeof(sockaddr),
						  &got);
			if (err != 0) {
				printf("TFTP: request sendto failed: 0x%x\n",
				       err);
				goto abort;
			}
			if (got != reqsize) {
				printf("TFTP: request truncated: %d != %d\n",
				       got, reqsize);
				goto abort;
			}
			sendreq = 0;
		}

		/*
		 * Read the response.
		 * All sockets are nonblocking right now, so we read and spin.
		 * Give the server half a second to respond before retrying.
		 */
		for (i = 0; i < 5000; i++) {
			slen = sizeof(sockaddr);
			err = oskit_socket_recvfrom(srvsock, replybuf,
						    sizeof(replybuf), 0,
						    &sockaddr, &slen, &got);
			if (err != OSKIT_EWOULDBLOCK)
				break;

			osenv_timer_spin(100000); /* 100us */
		}

		/*
		 * No response from the server, send the request/ACK again
		 */
		if (err == OSKIT_EWOULDBLOCK) {
			dprintf("TFTP: no response from server\n");
			sendreq = 1;
			continue;
		}

		/*
		 * Other failure, abort
		 */
		if (err != 0) {
			printf("TFTP: reply recvfrom failed: 0x%x\n", err);
			goto abort;
		}
		got -= (2 * sizeof(unsigned short));
		if (got < 0) {
			printf("TFTP: reply truncated\n");
			goto abort;
		}

		/*
		 * Feeb spoofing check
		 */
		if (sin->sin_addr.s_addr != srvaddr.s_addr) {
			printf("TFTP: reply from wrong server %s\n",
			       inet_ntoa(sin->sin_addr));
			goto abort;
		}

		if (sin->sin_port != srvport) {
			srvport = sin->sin_port;
			dprintf("TFTP: using port %d\n", ntohs(srvport));
		}

		/*
		 * Got a legit response.
		 * If it is an error, return.  Note that we don't
		 * abort since the server knows.
		 */
		reply = (struct tftphdr *)replybuf;
		reply->th_opcode = ntohs(reply->th_opcode);
		if (reply->th_opcode == ERROR) {
			printf("TFTP: %s (error %d)\n",
			       reply->th_msg, reply->th_code);
			err = reply->th_code;
			goto abort;
		}

		/*
		 * Got some data, record it as the current block and ACK
		 */
		if (reply->th_opcode == DATA) {
			reply->th_block = ntohs(reply->th_block);
			if (reply->th_block != currentblock+1) {
				dprintf("TFTP: received wrong block %d\n",
				       reply->th_block);
				if (reply->th_block == currentblock) {
					sendreq = 1;
					continue;
				}
				goto abort;
			}
			currentblock = reply->th_block;
			currentdata = reply->th_data;
			currentsize = got;

			req->th_opcode = htons((u_short)ACK);
			req->th_block = htons(currentblock);
			reqsize = 2 * sizeof(unsigned short);

			slen = sizeof(sockaddr);
			sin->sin_addr.s_addr = srvaddr.s_addr;
			sin->sin_port = srvport;
			err = oskit_socket_sendto(srvsock, req, reqsize, 0,
						  &sockaddr, slen, &got);
			if (err != 0) {
				printf("TFTP: ACK failed: 0x%x\n", err);
				goto abort;
			}
			if (got != reqsize) {
				printf("TFTP: ACK truncated: %d != %d\n",
				       got, reqsize);
				goto abort;
			}

			return 0;
		}

		/*
		 * Not a DATA reply, what should we do?
		 * Standard BSD tftp, just keeps receiving and so do we.
		 */
		dprintf("TFTP: expecting DATA packet, got type %d\n",
			reply->th_opcode);
	}

 abort:
	/*
	 * If we get here, we hit our retry limit or aborted for some
	 * reason.  We should do something sensible.
	 */
	return err;
}

/*
 * Big Stupid Cache Hack.
 *
 * TFTP is strictly a sequential I/O protocol, but netboot doesn't behave
 * that way.  We need to read the first MULTIBOOT_SEARCH bytes of the file
 * twice, once when scanning for the multiboot header and once when loading
 * the exec image.  Moreover, the exec library code makes partial block and
 * block unaligned requests.
 *
 * However, we can't use a straightforward cache.  Once something has been
 * flushed, it is gone forever.  It just isn't practical to reload it
 * (would have to issue a new Read Request and start over from the beginning
 * of the file).  We could just cache the entire file, but that cuts in
 * half the size of a kernel that can be loaded and we really don't do
 * much random access.
 *
 * So our cache operates in two phases.  In phase 1 it caches everything
 * less than MULTIBOOT_SEARCH, throwing away nothing.  The cache is big
 * enough to hold all these blocks.  This addresses the reuse of the first
 * 8k or so of the file.
 *
 * When we first see a file offset larger than MULTIBOOT_SEARCH, we flush
 * the cache and switch to phase two.  In phase two, we do no "cache behind".
 * We cache blocks whenever we have a partial block reference, but only until
 * we use the last byte of the block at which point we flush the entry.
 * The assumption is that we are sequentially moving forward through the file
 * at this point, but just not always making full-block requests (exec lib
 * reading the ELF headers) or making non-block-aligned full-block requests
 * (a mkmbimage encapsulated kernel where the initial MB header throws the
 * executable out of alignment).
 */
static struct bsch {
	int blkno;
	int len;
	void *ptr;
} bsch[MULTIBOOT_SEARCH/SEGSIZE+8];
static int bschents = sizeof(bsch) / sizeof(bsch[0]);
static int bschphase = 1;

static void
bsch_store(int blkno, oskit_addr_t reqoff, oskit_size_t reqsize,
	   void *data, int datasize)
{
	struct bsch *ent = 0;
	int i;

	assert(datasize <= SEGSIZE);

	if (bschphase == 1) {
		/*
		 * Always cache as we must be in the MULTIBOOT_SEARCH zone
		 */
		assert(reqoff < MULTIBOOT_SEARCH);
	}
	if (bschphase == 2) {
		/*
		 * Aligned, full-block requests are not cached
		 */
		if ((reqoff % SEGSIZE) == 0 && reqsize >= SEGSIZE)
			return;
	}

	/*
	 * Now that we know we will cache it, make sure we have an entry
	 */
	for (i = 0; i < bschents; i++) {
		if (ent == 0 && bsch[i].blkno == 0)
			ent = &bsch[i];
		else if (bsch[i].blkno == blkno) {
			ent = &bsch[i];
			break;
		}
	}
	if (ent == 0)
		panic("bsch_store: no cache entry available");

	if (ent->ptr == 0)
		ent->ptr = mustmalloc(SEGSIZE);

#if 0
	printf(" bsch(%d): storing block %d, size=%d (reqoff=%d, reqsize=%d)\n",
	       bschphase, blkno, datasize, reqoff, reqsize);
#endif
	ent->blkno = blkno;
	memcpy(ent->ptr, data, datasize);
	ent->len = datasize;
}

static int
bsch_load(int blkno, oskit_addr_t reqoff, oskit_size_t reqsize, void *data)
{
	struct bsch *ent = 0;
	int i, len, flush, blkoff;

	blkoff = reqoff % SEGSIZE;
	flush = 0;

	if (bschphase == 1) {
		if (reqoff >= MULTIBOOT_SEARCH) {
			flush = -1;
			bschphase = 2;
		}
	}
	if (bschphase == 2) {
		assert(reqoff >= MULTIBOOT_SEARCH);
		if (blkoff + reqsize >= SEGSIZE)
			flush = 1;
	}

	for (i = 0; i < bschents; i++) {
		if (bsch[i].blkno == blkno) {
			ent = &bsch[i];
			break;
		}
	}
	if (ent == 0 || ent->ptr == 0 || blkoff >= ent->len)
		return -1;

	len = reqsize;
	if (len > ent->len - blkoff)
		len = ent->len - blkoff;
#if 0
	printf(" bsch(%d): loading block %d, offset=%d, size=%d",
	       bschphase, blkno, blkoff, len);
#endif
	memcpy(data, ent->ptr + blkoff, len);

	if (flush) {
#if 0
		printf(" flushing %s", flush > 0 ? "entry" : "cache");
#endif
		if (flush > 0)
			ent->blkno = 0;
		else
			bsch_flush();
	}
#if 0
	printf("\n");
#endif

	return len;
}

static void
bsch_flush(void)
{
	int i;

	for (i = 0; i < bschents; i++) {
		bsch[i].blkno = 0;
		if (bsch[i].ptr) {
			free(bsch[i].ptr);
			bsch[i].ptr = 0;
		}
	}
}

#if 0
int
datasum(void *buf, int len)
{
	int *ip = buf, sum = 0;
	int i;

	for (i = 0; i < len/4; i++)
		sum ^= ip[i];
	return sum;
}
#endif
#endif
