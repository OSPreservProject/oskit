/* Modified from FreeBSD 2.1 sys/i386/boot/netboot */
/***********************************************************************

Remote Procedure Call Support Routines

Author: Martin Renters
  Date: Oct/1994

***********************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <oskit/dev/dev.h>

#include "misc.h"
#include "driver.h"
#include "socket.h"
#include "timer.h"
#include "rpc.h"

/*
 * Once a file is opened, stash the socket and address here, to be used
 * on subsequent reads. There are better ways to do this of course.
 */
static oskit_socket_t		*rpcsock;
static struct oskit_sockaddr	rpcsockaddr;

#define BUFMAX	32	/* should not exceed PACKQ_MAX in driver.c */

struct rpc_buf read_cache[BUFMAX];
int read_cache_size = 0;

int rpc_id;

void
rpc_init()
{
	int i;
	rpc_id = currticks();
	for (i = 0; i < BUFMAX; i++) {
		read_cache[i].valid = 0;
		read_cache[i].offset = 0;
		read_cache[i].len = 0;
	}
	if (rpcsock) {
		oskit_socket_release(rpcsock);
		rpcsock = 0;
	}
}

void
rpc_shutdown()
{
	if (rpcsock) {
		oskit_socket_release(rpcsock);
		rpcsock = 0;
	}
}

/***************************************************************************

RPC_LOOKUP:  Lookup RPC Port numbers

***************************************************************************/
int
rpc_lookup(struct in_addr server, int prog, int ver)
{
	struct rpc_t buf, *rpc = &buf;
	char *rpcptr;
	int retries = MAX_RPC_RETRIES;
	oskit_socket_t *sock;
	oskit_size_t got, slen;
	oskit_error_t err;
	struct oskit_sockaddr sockaddr;
	struct sockaddr_in    *sin = (void *) &sockaddr;
	int retval = -1;

	/*
	 * Create a socket to talk to the server. Use that for
	 * all RPC request traffic.
	 */
	if (comsocket(OSKIT_AF_INET, OSKIT_SOCK_DGRAM, IPPROTO_UDP, &sock)) {
		printf("rpc_lookup: comsocket failed\n");
		return -1;
	}

	/* Bind our side */
	sin->sin_family      = OSKIT_AF_INET;
	sin->sin_addr.s_addr = htonl(INADDR_ANY);
	sin->sin_port        = htons(RPC_SOCKET);
	if (oskit_socket_bind(sock, &sockaddr, sizeof(sockaddr))) {
		oskit_socket_release(sock);
		printf("rpc_lookup: oskit_socket_bind failed\n");
		return -1;
	}

	rpcptr = netsprintf(buf.u.data,"%L%L%L%L%L%L%L%L%L%L%L%L%L%L",
		rpc_id, MSG_CALL, 2, PROG_PORTMAP, 2, PORTMAP_LOOKUP,
		0, 0, 0, 0, prog, ver, IPPROTO_UDP, 0);

	while(retries--) {
		sin->sin_addr.s_addr = htonl(server.s_addr);
		sin->sin_port        = htons(SUNRPC);

		if ((err = oskit_socket_sendto(sock, &buf,
				rpcptr - (char *)&buf,
				0, &sockaddr, sizeof(sockaddr), &got))) {
			printf("rpc_lookup: sendto failed: 0x%x\n", err);
			goto done;
		}

		/* All sockets are nonblocking right now! */
		osenv_timer_spin(20000000);

		slen = sizeof(sockaddr);
		if ((err = oskit_socket_recvfrom(sock, &buf, sizeof(buf), 0,
						 &sockaddr, &slen, &got))) {
			if (err == OSKIT_EWOULDBLOCK)
				continue;
			printf("rpc_lookup: recvfrom failed: 0x%x\n", err);
			goto done;
		}
		/* Musy have received a reply! */
		if (rpc->u.reply.rstatus == rpc->u.reply.verifier ==
		    rpc->u.reply.astatus == 0) {
			retval = ntohl(rpc->u.reply.data[0]);
		}
		else {
			printf("rpc_lookup: %s\n", rpc_strerror(rpc));
		}
		break;
	}
 done:
	oskit_socket_release(sock);
	return retval;
}

/***************************************************************************

NFS_MOUNT:  Mount an NFS Filesystem

***************************************************************************/
int
nfs_mount(struct in_addr server, int port, char *path, char *fh)
{
	struct	rpc_t buf, *rpc = &buf;
	char	*rpcptr;
	int retries = MAX_RPC_RETRIES;
	oskit_socket_t *sock;
	oskit_size_t got, slen;
	oskit_error_t err;
	struct oskit_sockaddr sockaddr;
	struct sockaddr_in    *sin = (void *) &sockaddr;
	int retval = -1;
	
	/*
	 * Create a socket to talk to the server. Use that for
	 * all RPC request traffic.
	 */
	if (comsocket(OSKIT_AF_INET, OSKIT_SOCK_DGRAM, IPPROTO_UDP, &sock)) {
		printf("comsocket failed\n");
		return -1;
	}

	/* Bind our side */
	sin->sin_family      = OSKIT_AF_INET;
	sin->sin_addr.s_addr = htonl(INADDR_ANY);
	sin->sin_port        = htons(RPC_SOCKET);
	if (oskit_socket_bind(sock, &sockaddr, sizeof(sockaddr))) {
		oskit_socket_release(sock);
		printf("nfs_mount: oskit_socket_bind failed\n");
		return -1;
	}

	rpcptr = netsprintf(buf.u.data,"%L%L%L%L%L%L%L%L%L%S%L%L%L%L%L%L%L%S",
		rpc_id, MSG_CALL, 2, PROG_MOUNT, 1, MOUNT_ADDENTRY,
		1, hostnamelen + 28,0,hostname,0,0,2,0,0,0,0,
		path);
	
	while(retries--) {
		sin->sin_addr.s_addr = htonl(server.s_addr);
		sin->sin_port        = htons(port);

		if ((err = oskit_socket_sendto(sock, &buf,
				rpcptr - (char *)&buf,
			        0, &sockaddr, sizeof(sockaddr), &got))) {
			printf("nfs_mount: sendto failed: 0x%x\n", err);
			goto done;
		}

		/* All sockets are nonblocking right now! */
		osenv_timer_spin(20000000);

		slen = sizeof(sockaddr);
		if ((err = oskit_socket_recvfrom(sock, &buf, sizeof(buf), 0,
						 &sockaddr, &slen, &got))) {
			if (err == OSKIT_EWOULDBLOCK)
				continue;
			printf("nfs_mount: recvfrom failed: 0x%x\n", err);
			goto done;
		}
		/* Musy have received a reply! */
		if (rpc->u.reply.rstatus || rpc->u.reply.verifier ||
		    rpc->u.reply.astatus || rpc->u.reply.data[0]) {
			printf("nfs_mount: %s\n", rpc_strerror(rpc));
			retval = -(ntohl(rpc->u.reply.data[0]));
		} else {
			memcpy(fh, &rpc->u.reply.data[1], 32);
			retval = 0;
		}
		break;
	}
 done:
	oskit_socket_release(sock);
	return retval;
}

/***************************************************************************

NFS_LOOKUP:  Lookup Pathname

***************************************************************************/
int
nfs_lookup(struct in_addr server, int port,
	   char *fh, char *path, char *file_fh)
{
	struct	rpc_t buf, *rpc = &buf;
	char	*rpcptr;
	int retries = MAX_RPC_RETRIES;
	oskit_socket_t *sock;
	oskit_size_t got, slen;
	oskit_error_t err;
	struct oskit_sockaddr sockaddr;
	struct sockaddr_in    *sin = (void *) &sockaddr;
	int retval = -1;
	
	/*
	 * Create a socket to talk to the server. Use that for
	 * all RPC request traffic.
	 */
	if (comsocket(OSKIT_AF_INET, OSKIT_SOCK_DGRAM, IPPROTO_UDP, &sock)) {
		printf("nfs_lookup: comsocket failed\n");
		return -1;
	}

	/* Bind our side */
	sin->sin_family      = OSKIT_AF_INET;
	sin->sin_addr.s_addr = htonl(INADDR_ANY);
	sin->sin_port        = htons(RPC_SOCKET);
	if (oskit_socket_bind(sock, &sockaddr, sizeof(sockaddr))) {
		oskit_socket_release(sock);
		printf("nfs_lookup: oskit_socket_bind failed\n");
		return -1;
	}

	rpcptr = netsprintf(buf.u.data,
			    "%L%L%L%L%L%L%L%L%L%S%L%L%L%L%L%L%L%M%S",
			    rpc_id, MSG_CALL, 2, PROG_NFS, 2, NFS_LOOKUP,
			    1, hostnamelen + 28,0,hostname,0,0,2,0,0,0,0,
			    32, fh, path);
	
	while(retries--) {
		sin->sin_addr.s_addr = htonl(server.s_addr);
		sin->sin_port        = htons(port);

		if ((err = oskit_socket_sendto(sock, &buf,
				rpcptr - (char *)&buf,
				0, &sockaddr, sizeof(sockaddr), &got))) {
			printf("nfs_lookup: sendto failed: 0x%x\n", err);
			goto done;
		}

		/* All sockets are nonblocking right now! */
		osenv_timer_spin(20000000);

		slen = sizeof(sockaddr);
		if ((err = oskit_socket_recvfrom(sock, &buf, sizeof(buf), 0,
						 &sockaddr, &slen, &got))) {
			if (err == OSKIT_EWOULDBLOCK)
				continue;
			printf("nfs_lookup: recvfrom failed: 0x%x\n", err);
			goto done;
		}
		/* Musy have received a reply! */
		if (rpc->u.reply.rstatus || rpc->u.reply.verifier ||
		    rpc->u.reply.astatus || rpc->u.reply.data[0]) {
			printf("nfs_lookip: %s\n", rpc_strerror(rpc));
			retval = -(ntohl(rpc->u.reply.data[0]));
		} else {
			memcpy(file_fh, &rpc->u.reply.data[1], 32);
			retval = 0;
		}
		break;
	}
	if (!retval) {
		/*
		 * Stash socket and address away for NFS reads.
		 */
		rpcsockaddr = sockaddr;
		rpcsock = sock;
		return 0;
	}
 done:
	oskit_socket_release(sock);
	return retval;
}

/***************************************************************************

CHECK_READ_CACHE:  Determine if a block is in the read cache

***************************************************************************/

static int read_cache_empty   = 1;
static int read_cache_filling = 0;

int
check_read_cache(int offset, int len, char *buffer)
{
	int i;
	struct rpc_buf *cache;

	for (i = 0; i < BUFMAX; i++) {
		cache = &(read_cache[i]);
		if (cache->valid &&
		    cache->offset == offset &&
		    cache->len    >= len) {
			memcpy(buffer, cache->buffer, len);
			if (i == BUFMAX-1)
				read_cache_empty = 1;
			return len;
		}
	}

	return 0;
}

void
read_cache_requests(void)
{
	oskit_size_t got;
	int i;

	for (i = 0; i < BUFMAX; i++) {
		struct rpc_buf *cache = &(read_cache[i]);
			
		if (cache->valid == 0 && cache->len != 0) {
			oskit_socket_sendto(rpcsock,
					    cache->buffer, cache->reqlen, 0,
					    &rpcsockaddr, sizeof(rpcsockaddr),
					    &got);
		}
	}
}

void
clear_read_cache(void)
{
	int i;

	/*
	 * If there are outstanding requests, there might also be replies
	 * in the UDP queue. We want to make sure those are tossed, but
	 * we have to be careful since if a reply did get lost, and
	 * read_cache_filling is non-zero, we will loop waiting for a reply
	 * that never comes cause we never make the request.
	 */
	for (i = 0; i < BUFMAX; i++) {
		read_cache[i].valid = 0;
		read_cache[i].len   = 0;
		read_cache[i].id    = 0;
	}
	read_cache_filling = 0;
}

int
read_cache_wait(int offset, int len)
{
	int i, rid, rlen;
	struct	rpc_t buf, *rpc = &buf;
	struct rpc_buf *cache;
	oskit_size_t got, slen;
	oskit_error_t err;
	int retries = 200;

	while (read_cache_filling) {
		if (retries-- < 0) {
			printf("NFS read timeout. Retrying ... \n");
		tryagain:
			read_cache_requests();
			retries = 200;
		}
		
		slen = sizeof(rpcsockaddr);
		if ((err = oskit_socket_recvfrom(rpcsock, &buf, sizeof(buf), 0,
						 &rpcsockaddr, &slen, &got))) {
			if (err == OSKIT_EWOULDBLOCK ||
			    err == OSKIT_EAGAIN) {
				/* All sockets are nonblocking right now! */
				osenv_timer_spin(10000000);
				continue;
			}
			return -1;
		}
		if (rpc->u.reply.rstatus || rpc->u.reply.verifier ||
		    rpc->u.reply.astatus || rpc->u.reply.data[0]) {
			/*
			 * No idea what this is or why. Just retry
			 * and hope it goes away.
			 */
			if (ntohl(rpc->u.reply.data[0]) == 13) {
				goto tryagain;
			}
			printf("%s %ld\n", rpc_strerror(rpc),
			       -(ntohl(rpc->u.reply.data[0])));
			panic("bar");
			return(-(ntohl(rpc->u.reply.data[0])));
		} else {
			rid = ntohl(rpc->u.reply.id);
			cache = NULL;
			for (i = 0; i < BUFMAX; i++) {
				if (read_cache[i].id == rid) {
					cache = &(read_cache[i]);
				}
			}
			if (cache == NULL) {
				continue;
			}
			if (cache->valid) {
				continue;
			}
			rlen = ntohl(rpc->u.reply.data[18]);
			if (cache->len < rlen) rlen = cache->len;
			if (rlen > 0) {
				if (cache->len > rlen)
					netprintf("short read\n");
				memcpy(cache->buffer,
				       &rpc->u.reply.data[19],
				       rlen);
			}
			cache->len = rlen;
			cache->valid = 1;
			read_cache_filling--;
		}
	}
	read_cache_empty = 0;
	return 0;
}	

/***************************************************************************

NFS_READ:  Read File

***************************************************************************/
int
nfs_read(struct in_addr server,
	 int port, char *fh, int offset, int len, char *buffer,
	 int readahead)
{
	struct	rpc_t buf, *rpc = &buf;
	char	*rpcptr;
	int	retries = MAX_RPC_RETRIES;
	int	rlen;
	int     curoff, pktcount = 0;
	int     saved_rpc_id;
	struct rpc_buf *cache;
	oskit_size_t got, slen;
	oskit_error_t err;

	if (read_cache_filling)
		read_cache_wait(offset, len);

	rlen = check_read_cache(offset, len, buffer);

	if (!rlen || read_cache_empty) {
		readahead = min(readahead, BUFMAX);
		saved_rpc_id = rpc_id;

		if (readahead < 1) readahead = 1;

		if (!rlen)
			curoff = offset;
		else
			curoff = offset + len;

		while (readahead) {
			cache = &(read_cache[rpc_id - saved_rpc_id]);
			cache->valid  = 0;
			cache->offset = curoff;
			cache->id     = rpc_id;
			cache->len    = len;

			rpcptr = netsprintf(cache->buffer,
				"%L%L%L%L%L%L%L%L%L%S%L%L%L%L%L%L%L%M%L%L%L",
				rpc_id, MSG_CALL, 2, PROG_NFS, 2, NFS_READ,
				1, hostnamelen + 28,0,hostname,0,0,2,0,0,0,0,
				32, fh, curoff, len, 0);
			cache->reqlen = rpcptr - cache->buffer;

			curoff += len;
			readahead--;
			pktcount++;
			rpc_id++;
		}
		read_cache_requests();
		read_cache_filling = pktcount;
	}
	
	if (rlen) {
		return rlen;
	}

	read_cache_wait(offset, len);

	rlen = check_read_cache(offset, len, buffer);
	if (rlen) {
		return rlen;
	}
	panic("read cache");

	/*
	 * Okay - fall back to normal behavior.  We missed the
	 * packet somehow.
	 */

	printf("nfs_read: lost packet\n");

	while(retries--) {
		rpcptr = netsprintf(buf.u.data,
		        "%L%L%L%L%L%L%L%L%L%S%L%L%L%L%L%L%L%M%L%L%L",
			rpc_id, MSG_CALL, 2, PROG_NFS, 2, NFS_READ,
		        1, hostnamelen + 28,0,hostname,0,0,2,0,0,0,0,
		        32, fh, offset, len, 0);

		oskit_socket_sendto(rpcsock, &buf,
				    rpcptr - (char *)&buf, 0,
				    &rpcsockaddr, sizeof(rpcsockaddr), &got);

		/* All sockets are nonblocking right now! */
		osenv_timer_spin(20000000);

		slen = sizeof(rpcsockaddr);
		if ((err = oskit_socket_recvfrom(rpcsock, &buf, sizeof(buf), 0,
						 &rpcsockaddr, &slen, &got))) {
			if (err == OSKIT_EWOULDBLOCK)
				continue;
			return -1;
		}
		/* Musy have received a reply! */
		if (rpc->u.reply.rstatus || rpc->u.reply.verifier ||
		    rpc->u.reply.astatus || rpc->u.reply.data[0]) {
			printf("%s\n", rpc_strerror(rpc));
			return(-(ntohl(rpc->u.reply.data[0])));
		} else {
			rlen = ntohl(rpc->u.reply.data[18]);
			if (len < rlen) rlen = len;
			if (rlen > 0) {
				if (len > rlen)
					netprintf("short read\n");
				memcpy(buffer, &rpc->u.reply.data[19], rlen);
			}
			return(rlen);
		}
	}
	return(-1);
}

char *
rpc_strerror(struct rpc_t *rpc)
{
	static char buf[64];

	sprintf(buf, "RPC Error: (%ld,%ld,%ld)",
		ntohl(rpc->u.reply.rstatus),
		ntohl(rpc->u.reply.verifier),
		ntohl(rpc->u.reply.astatus));
	return buf;
}

char *
nfs_strerror(int err)
{
	static char buf[64];

	switch (-err) {
	case NFSERR_PERM:	return "Not owner";
	case NFSERR_NOENT:	return "No such file or directory";
	case NFSERR_ACCES:	return "Permission denied";
	default:
		sprintf(buf, "Unknown NFS error %d", -err);
		return buf;
	}
}
