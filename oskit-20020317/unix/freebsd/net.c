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
#include <oskit/dev/dev.h>
#include <oskit/dev/error.h>
#include <oskit/dev/freebsd.h>
#include <oskit/dev/clock.h>
#include <oskit/dev/timer.h>
#include <oskit/com/listener.h>
#include <oskit/dev/net.h>
#include <oskit/dev/ethernet.h>
#include <oskit/net/freebsd.h>
#include <oskit/net/socket.h>
#include <oskit/io/netio_fanout.h>
#include <oskit/io/bufio.h>

#include "native.h"
#include "native_errno.h"
#include "support.h"

#include <sys/types.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/ioccom.h>
#include <net/bpf.h>
#include <net/if.h>
#include <netinet/if_ether.h>
#include <arpa/inet.h>

/**************************************************************************/


/**************************************************************************/

/*** Network send I/O interface ***/

typedef struct bpfnetdev {
	oskit_netio_t	nio;
	oskit_etherdev_t	eio;
	unsigned	count;
	int		fd;
	oskit_netio_t	*recv;
	unsigned char	ethaddr[OSKIT_ETHERDEV_ADDR_SIZE];
	char		name[6];
	unsigned char	*recvbuf;
	unsigned int	buflen;
} bpfnetdev_t;

/*
 * Query a net I/O object for its interfaces.
 */
static OSKIT_COMDECL
net_query(oskit_netio_t *io, const oskit_iid_t *iid, void **out_ihandle)
{
	bpfnetdev_t *dev = (bpfnetdev_t *)io;

	if (dev == NULL)
		panic("%s:%d: null peropen", __FILE__, __LINE__);
	if (dev->count == 0)
		panic("%s:%d: bad count", __FILE__, __LINE__);

	if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_netio_iid, sizeof(*iid)) == 0) {
		*out_ihandle = &dev->nio;
		++dev->count;
		return 0;
	}

	*out_ihandle = NULL;
	return OSKIT_E_NOINTERFACE;
}

/*
 * Clone a reference to a device's block I/O interface.
 */
static OSKIT_COMDECL_U
net_addref(oskit_netio_t *io)
{
	bpfnetdev_t *dev = (bpfnetdev_t *)io;

	if (dev == NULL)
		panic("%s:%d: null peropen", __FILE__, __LINE__);
	if (dev->count == 0)
		panic("%s:%d: bad count", __FILE__, __LINE__);

	return ++dev->count;
}


/*
 * Close ("release") a device.
 */
static OSKIT_COMDECL_U
net_release(oskit_netio_t *io)
{
	bpfnetdev_t *dev = (bpfnetdev_t *)io;
	unsigned newcount;

	if (dev == NULL)
		panic("%s:%d: null peropen", __FILE__, __LINE__);
	if (dev->count == 0)
		panic("%s:%d: bad count", __FILE__, __LINE__);

	newcount = dev->count - 1;
	if (newcount == 0) {

		oskitunix_unregister_async_fd(dev->fd);
		NATIVEOS(close)(dev->fd);
		dev->fd = 0;

		/*
		 * Release our reference on the client's
		 * receive net_io interface.
		 */
		oskit_netio_release(dev->recv);
		dev->recv = NULL;
	}

	return dev->count = newcount;
}


/*
 * Queue a packet for transmission.
 */
static OSKIT_COMDECL
net_push(oskit_netio_t *io, oskit_bufio_t *b, oskit_size_t size)
{
        bpfnetdev_t *dev = (bpfnetdev_t *)io;
	int err, maperr;
	oskit_size_t actual;
	void *p;

	/*
	 * Try to map the packet into contiguous local memory.
	 * Since the map operation on net_io interfaces is optional,
	 * we have to use copyin instead if this doesn't work.
	 */
	maperr = oskit_bufio_map(b, &p, 0, size);

	if (maperr) {
		p = osenv_mem_alloc(size, OSENV_NONBLOCKING, 0);

		/* Couldn't map, try to copy it in */
	        err = oskit_bufio_read(b, p, 0, size, &actual);
		assert(actual == size);
	}
/*
 * note: this must reflect the properties of the system you'll run it on,
 * not the system you compile it on.
 */
#if BSD == 199306	/* 2.1.5 is broken */
	{
		struct ether_header *eh = (void *)p;
		eh->ether_type = ntohs(eh->ether_type);
		if (VERBOSE()) {
			osenv_log(OSENV_LOG_DEBUG,
				  "send packet type 0x%x, len=%d\n",
				  eh->ether_type, size);
		}
	}
#elif BSD == 199506	/* 2.2.2 is fixed */
	/* don't switch the ethertype */
#else
#error Does this version erroneously switch the ethernet type for raw packets?
#endif
	/*
	 * also, not that the source address is overwritten
	 * the BSD kernel doesn't allow to forge ethernet source addresses
	 * so make sure our code doesn't even attempt it:
	 */
	assert(!memcmp(p+6, dev->ethaddr, 6));
#if 0
	printf("write packet with %d bytes\n", size);
	hexdump(p, 48);
#endif
	do {
		err = NATIVEOS(write)(dev->fd, p, size);
	} while (err == -1 && NATIVEOS(errno) == EWOULDBLOCK);

	if (err == -1)
		printf("freebsd %s: error sending packet: (%s)\n",
		       __FUNCTION__, 
		       strerror(native_to_oskit_error(NATIVEOS(errno))));
	
	if (!maperr)
		oskit_bufio_unmap(b, p, 0, size);
	else
		osenv_mem_free(p, OSENV_NONBLOCKING, size);

	return err == size ? 0 : err;
}

/*
 * Unimplemented bufio allocator
 */
static OSKIT_COMDECL
net_alloc_bufio(oskit_netio_t *io, oskit_size_t size,
		oskit_bufio_t **out_bufio)
{
	return OSKIT_E_NOTIMPL;
}

/**************************************************************************/


/**************************************************************************/


/*** Network device node interface methods ***/

static OSKIT_COMDECL
netdev_query(oskit_etherdev_t *io, const oskit_iid_t *iid, void **out_ihandle)
{
        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_device_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_etherdev_iid, sizeof(*iid)) == 0) {
                *out_ihandle = io;
                return 0;
        }

        *out_ihandle = NULL;
        return OSKIT_E_NOINTERFACE;
};

static OSKIT_COMDECL_U
netdev_addref(oskit_etherdev_t *intf)
{
	/* No reference counting */
	return 1;
}

static OSKIT_COMDECL_U
netdev_release(oskit_etherdev_t *intf)
{
	/* No reference counting */
	return 1;
}

static OSKIT_COMDECL
netdev_getinfo(oskit_etherdev_t *intf, oskit_devinfo_t *out_info)
{
	bpfnetdev_t *dev = (bpfnetdev_t *)(intf-1);
	out_info->name = dev->name;
	out_info->description = "The fantastic /dev/bpf networking device";
	out_info->vendor = "UofU";
	out_info->author = "Godmar Back";
	out_info->version =  "1.0";
	return 0;
}

static OSKIT_COMDECL
netdev_getdriver(oskit_etherdev_t *intf, oskit_driver_t **out_driver)
{
	return OSKIT_ENOTSUP;
}

/*
 * read all packets you can from this interface
 */
static void
read_packets(void *_dev)
{
	bpfnetdev_t	*dev = _dev;
	oskit_error_t  	err;
	oskit_bufio_t 	*b;
	int		r, ofs;
	struct bpf_hdr  *hdr = (void *)dev->recvbuf;

	r = NATIVEOS(read)(dev->fd, dev->recvbuf, dev->buflen);
	if (r == -1 && NATIVEOS(errno) != EWOULDBLOCK) {
		oskitunix_perror("read_packets");
		exit(0);
	}
	else if (r == 0 || (r == -1 && NATIVEOS(errno) == EWOULDBLOCK)) {
		osenv_log(OSENV_LOG_ERR,
			  "spurious wakeup (rc=%d `%s')\n",
			  r, r ? strerror(r):"");
		goto done;
	}

	/* else process what we got */
	for (ofs = 0; ofs < r;
			ofs += hdr->bh_hdrlen + hdr->bh_datalen,
			ofs = BPF_WORDALIGN(ofs))
	{
		void *p;
		int  len, outgoing, handup = 1;
		struct ether_header *eh;

		/* next header */
		hdr = (void *)(dev->recvbuf + BPF_WORDALIGN(ofs));
		len = hdr->bh_datalen;		/* total packet length */

		b = oskit_bufio_create(len);
		err = oskit_bufio_map(b, &p, 0, len);
		assert(!err);
#if 1   /* all this is not needed */
		memcpy(p, dev->recvbuf + ofs + hdr->bh_hdrlen, len);
		eh = p;
		outgoing = !memcmp(eh->ether_shost, dev->ethaddr, 6);
#if 0	/* reject outgoing packets */
		if (outgoing)
			handup = 0;
#endif

#if 0	/* reject IEEE 802.3 */
		if (ntohs(eh->ether_type) < 1500)
			handup = 0;
#endif
#if 0	/* be verbose */
		if (v = VERBOSE()) {
		    osenv_log(OSENV_LOG_ERR, outgoing ? "OUT:" : "IN:");
		    osenv_log(OSENV_LOG_ERR, handup ? "HANDUP:" : "REJECT:");
		    osenv_log(OSENV_LOG_ERR, " type 0x%x, len=%d\n",
			    ntohs(eh->ether_type), hdr->bh_datalen);
		    if (v > 3 && handup)
			    dohexdump(0 /* base */,
				p, hdr->bh_datalen, 0 /* bytes */);
		}
#endif
#endif
		err = oskit_bufio_unmap(b, p, 0, len);
		assert(!err);

		if (handup)
			oskit_netio_push(dev->recv, b, len);
	}

done:
	/* please call read_packets(dev) when we can read */
	oskitunix_register_async_fd(dev->fd, IOTYPE_READ, read_packets, _dev);
	return;
}

static OSKIT_COMDECL
netdev_open(oskit_etherdev_t *intf, unsigned flags,
	   oskit_netio_t *recv_net_io,
	   oskit_netio_t **out_send_net_io)
{
	bpfnetdev_t *dev = (bpfnetdev_t *)(intf-1);

	/* Only allow one opener at a time */
	if (dev->count > 0)
		return OSKIT_EBUSY;

	/* Stash a reference to the supplied recv_net_io interface */
	dev->recv = recv_net_io;
	oskit_netio_addref(recv_net_io);

	/* Caller starts out with one reference to the send_net_io interface */
	dev->count = 1;
	*out_send_net_io = &dev->nio;

	if (!dev->fd) {
		dev->fd = oskitunix_open_eth(dev->name, dev->ethaddr,
					&dev->buflen,
					/* don't filter out IEEE 802.3 */ 0);
		if (dev->fd < 0) {
			oskit_netio_release(recv_net_io);
			return OSKIT_EINVAL;	/* XXX */
		}
	}

	oskitunix_register_async_fd(dev->fd, IOTYPE_READ,
				    read_packets, (void *)dev);
	return 0;
}

static OSKIT_COMDECL
netdev_rxpoll(oskit_etherdev_t *fdev, int *count, oskit_bufio_t *out_bufios[])
{
	return OSKIT_ENODEV;
}

static OSKIT_COMDECL_V
netdev_get_addr(oskit_etherdev_t *fdev,
	unsigned char out_addr[OSKIT_ETHERDEV_ADDR_SIZE])
{
	bpfnetdev_t *dev = (bpfnetdev_t *)(fdev-1);
	memcpy(out_addr, dev->ethaddr, OSKIT_ETHERDEV_ADDR_SIZE);
}

static struct oskit_netio_ops netio_ops = {
	net_query, net_addref, net_release,
	net_push, net_alloc_bufio
};

static struct oskit_etherdev_ops eth_ops = {
	netdev_query,
	netdev_addref,
	netdev_release,
	netdev_getinfo,
	netdev_getdriver,
	netdev_open,
	netdev_rxpoll,
	netdev_get_addr,
};

static int
add_ethernet_device(char *name)
{
	bpfnetdev_t *feth;
	int rc;

	feth = malloc(sizeof *feth);
	if (feth == 0)
		return OSKIT_ENOMEM;

	feth->nio.ops = &netio_ops;
	feth->eio.ops = &eth_ops;
	feth->count = 0;

	feth->fd = oskitunix_open_eth(name, feth->ethaddr, &feth->buflen, 0);
	if (feth->fd < 0) {
		free(feth);
		return OSKIT_EINVAL;	/* XXX */
	}

	feth->recvbuf = (unsigned char *)malloc(feth->buflen);
	if (feth->recvbuf == 0) {
		oskitunix_close_eth(feth->fd);
		free(feth);
		return OSKIT_ENOMEM;
	}

	strncpy(feth->name, name, sizeof feth->name);

	rc = osenv_device_register((oskit_device_t *)&feth->eio,
				   &oskit_etherdev_iid, 1);
	if (rc) {
		oskitunix_close_eth(feth->fd);
		free(feth->recvbuf);
		free(feth);
	}

	return rc;
}

/* Yeah, right */
static char default_devs[] = "de1";

void
oskit_linux_init_net()
{
	char	*devs;
	int	rc;
	static int init;

	if (init)
		return;
	init++;
	if (!(devs = getenv("ETHERIF")))
		devs = default_devs;
	while (devs != 0 && *devs != 0) {
		char *dev;

		dev = strsep(&devs, ",");
		rc = add_ethernet_device(dev);
		if (rc != 0)
			osenv_log(OSENV_LOG_ERR,
				  "%s: add_ethernet_device failed: %x\n",
				  dev, rc);
	}
}

#if 0
/* debugging */
int rv = 0;
int VERBOSE() { return rv; }
#endif
