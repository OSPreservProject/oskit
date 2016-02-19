/* Modified from FreeBSD 2.1 sys/i386/boot/netboot */
/***********************************************************************

Remote Procedure Call Support Routines

Author: Martin Renters
  Date: Oct/1994

***********************************************************************/

#ifndef __RPC_H_INCLUDED__
#define __RPC_H_INCLUDED__

#include <arpa/inet.h>
#include <netinet/in.h>
#include <oskit/net/socket.h>

#define NFS_READ_SIZE		1024
#define MAX_RPC_RETRIES		20

struct rpc_t {
	union {
		char data[1400];
		struct {
			long id;
			long type;
			long rstatus;
			long verifier;
			long v2;
			long astatus;
			long data[1];
		} reply;
	} u;
};

struct rpc_buf {
	int valid;
	int reqlen;
	unsigned long offset;
	unsigned long len;
	unsigned long id;
	char buffer[1400];
};

#define SUNRPC		111

#define RPC_SOCKET	620			/* Arbitrary */

#define PROG_PORTMAP	100000
#define PROG_NFS	100003
#define PROG_MOUNT	100005

#define MSG_CALL	0
#define MSG_REPLY	1

#define PORTMAP_LOOKUP	3

#define MOUNT_ADDENTRY	1
#define NFS_LOOKUP	4
#define NFS_READ	6

#define NFS_FHSIZE	32

#define NFSERR_PERM	1
#define NFSERR_NOENT	2
#define NFSERR_ACCES	13

extern int rpc_id;

void rpc_init(void);
int rpc_lookup(struct in_addr ipaddr, int prog, int ver);
int nfs_mount(struct in_addr ipaddr, int port, char *path, char *fh);
int nfs_lookup(struct in_addr ipaddr, int port,
	       char *fh, char *path, char *file_fh);
int nfs_read(struct in_addr ipaddr, int port,
	     char *fh, int offset, int len, char *buffer, int readahead);
char *rpc_strerror(struct rpc_t *rpc);
char *nfs_strerror(int err);

/* XXX Needs thought. */
void clear_read_cache(void);

#endif /* __RPC_H_INCLUDED__ */
