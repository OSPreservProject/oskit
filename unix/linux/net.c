/*
 * Copyright (c) 1997,1999,2000 University of Utah and the Flux Group.
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
#define __USE_BSD
#include <sys/types.h>

#include <oskit/dev/dev.h>
#include <oskit/dev/net.h>
#include <oskit/dev/ethernet.h>
#include <oskit/io/bufio.h>
#include <oskit/error.h>

#include "native.h"
#include "support.h"

#include <fcntl.h>
#include <string.h>
#include <net/if.h>
#include <netinet/if_ether.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

/**************************************************************************/


/**************************************************************************/

/*** Network send I/O interface ***/

typedef struct linuxnetdev {
	oskit_netio_t	nio;
	oskit_etherdev_t	eio;
	unsigned	count;
	int		fd;
	oskit_netio_t	*recv;
	unsigned char	ethaddr[OSKIT_ETHERDEV_ADDR_SIZE];
	char		name[6];
	unsigned char	*recvbuf;
	unsigned int	buflen;
} linuxnetdev_t;

/*
 * Query a net I/O object for its interfaces.
 */
static OSKIT_COMDECL
net_query(oskit_netio_t *io, const oskit_iid_t *iid, void **out_ihandle)
{
	linuxnetdev_t *dev = (linuxnetdev_t *)io;

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
	linuxnetdev_t *dev = (linuxnetdev_t *)io;

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
	linuxnetdev_t *dev = (linuxnetdev_t *)io;
	unsigned newcount;

	if (dev == NULL)
		panic("%s:%d: null peropen", __FILE__, __LINE__);
	if (dev->count == 0)
		panic("%s:%d: bad count", __FILE__, __LINE__);

	newcount = dev->count - 1;
	if (newcount == 0) {

		oskitunix_unregister_async_fd(dev->fd);
		oskitunix_close_eth(dev->fd);
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
        linuxnetdev_t *dev = (linuxnetdev_t *)io;
	int err, maperr;
	oskit_size_t actual;
	void *p;
	struct sockaddr to;
	int tolen;

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

#if 0
	do {
		err = NATIVEOS(write)(dev->fd, p, size);
	} while (err == -1 && NATIVEOS(errno) == EWOULDBLOCK);
#else
	memset(&to, 0, sizeof(to));
	to.sa_family = AF_INET;
	strcpy(to.sa_data, dev->name);
	tolen = sizeof(to);

	do {
	    	err = NATIVEOS(sendto)(dev->fd, p, size, 0, &to, tolen);
	} while (err == -1 && NATIVEOS(errno) == EWOULDBLOCK);

	if (err == -1)
		printf("linux %s: error sending packet: (%s)\n",
		       __FUNCTION__, 
		       strerror(native_to_oskit_error(NATIVEOS(errno))));
#endif

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
	linuxnetdev_t *dev = (linuxnetdev_t *)(intf-1);
	out_info->name = dev->name;
	out_info->description = "linux networking device";
	out_info->vendor = "UofU";
	out_info->author = "Stephen Clawson";
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
	linuxnetdev_t	*dev = _dev;
	oskit_error_t  	err;
	oskit_bufio_t 	*b;
	struct sockaddr from;
	int		r, fromlen;

	/* Keep trying until we get one that's for us. =) */
	/* XXX Need to check to see if this will ever matter. */

retry:
	do {
	    fromlen = sizeof(from);
	    r = NATIVEOS(recvfrom)(dev->fd, dev->recvbuf, dev->buflen, 0,
				    &from, &fromlen);

	    if (r <= 0) {
		if (NATIVEOS(errno) == EAGAIN)
			goto retry;
		if (r == 0 || NATIVEOS(errno) == EWOULDBLOCK) {
		    osenv_log(OSENV_LOG_ERR,
			      "spurious wakeup (rc=%d `%s')\n",
			      r, r ? strerror(r):"");
		    goto done;
		} else {
		    oskitunix_perror("read_packets"),	exit(0);
		}
	    }
	} while (strcmp(dev->name, from.sa_data));

	{
	    	void *p;
		int  len, handup = 1;
#if 0
		struct ether_header *eh;
		eh = (void *)dev->recvbuf;
#endif
		len = r;

		b = oskit_bufio_create(len);
		err = oskit_bufio_map(b, &p, 0, len);
		assert(!err);

#if 0
		for (i = 0; i < 14; i++) {
		    osenv_log(OSENV_LOG_ERR, "%x ", dev->recvbuf[i]);
		}
		osenv_log(OSENV_LOG_ERR, "\n");
#endif
		memcpy(p, dev->recvbuf, len);
#if 0	/* be verbose */
		{
			int v = VERBOSE();
			if (v) {
				osenv_log(OSENV_LOG_ERR,
					  outgoing ? "OUT:" : "IN:");
				osenv_log(OSENV_LOG_ERR,
					  handup ? "HANDUP:" : "REJECT:");
				osenv_log(OSENV_LOG_ERR,
					  " type 0x%x, len=%d\n",
					  ntohs(eh->ether_type), fromlen);
				if (v > 3 && handup)
					dohexdump(0 /* base */,
						  p, fromlen, 0 /* bytes */);
			}
		}
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

int
oskitunix_open_eth(char *ifname, char *myaddr,
		   unsigned int *buflen, int no_ieee802_3)
{
	struct ifreq ifr;
	struct sockaddr sa;
	int fd, rc;
	char *bp, *msg;

	/* Open a socket to grab all packets. */
	fd = NATIVEOS(socket)(PF_INET, SOCK_PACKET, htons(ETH_P_ALL));
	if (fd < 0) {
		oskitunix_perror("socket(PF_INET, SOCK_PACKET, ...)");
		return -1;
	}

	/* Bind our socket to a specific interface. */
	memset(&sa, 0, sizeof(sa));
	sa.sa_family = AF_INET;
	(void)strncpy(sa.sa_data, ifname, sizeof(sa.sa_data));
	rc = NATIVEOS(bind)(fd, &sa, sizeof(sa));
	if (rc) {
		msg = "cannot bind to interface";
		goto failed;
	}

	if ((bp = (char *)getenv("ETHERADDR")) == NULL) {
		/* Get the MAC addr of our interface. */
		memset(&ifr, 0, sizeof(ifr));
		(void)strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
		rc = NATIVEOS(ioctl)(fd, SIOCGIFHWADDR, &ifr);
		if (rc < 0) {
			msg = "SIOCGIFHWADDR";
			goto failed;
		}
		memcpy(myaddr, ifr.ifr_hwaddr.sa_data, IFHWADDRLEN);
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

	memset(&ifr, 0, sizeof(ifr));
	(void)strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
	rc = NATIVEOS(ioctl)(fd, SIOCGIFMTU, &ifr);
	if (rc < 0) {
		msg = "SIOCGIFMTU";
		goto failed;
	}

	*buflen = ifr.ifr_mtu + 64;
	return fd;

 failed:
	if (msg)
		oskitunix_perror(msg);
	NATIVEOS(close)(fd);
	return -1;
}

void
oskitunix_close_eth(int fd)
{
	NATIVEOS(close)(fd);
}

static OSKIT_COMDECL
netdev_open(oskit_etherdev_t *intf, unsigned flags,
	   oskit_netio_t *recv_net_io,
	   oskit_netio_t **out_send_net_io)
{
	linuxnetdev_t *dev = (linuxnetdev_t *)(intf-1);
	int rc;

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
			dev->fd = 0;
			return OSKIT_EINVAL; /* XXX */
		}
	}

	oskitunix_register_async_fd(dev->fd, IOTYPE_READ,
				    read_packets, (void *)dev);

	/*
	 * We have to do this ourselves, since oskitunix_set_async_fd
	 * is only a stub function in the non-threaded library. =(
	 */
	rc = NATIVEOS(fcntl)(dev->fd, F_SETOWN, NATIVEOS(getpid)());
	if (rc >= 0)
		rc = NATIVEOS(fcntl)(dev->fd, F_SETFL, O_ASYNC | O_NONBLOCK);
	if (rc < 0) {
		oskitunix_perror("F_SETOWN/SETFL");
		oskitunix_close_eth(dev->fd);
		return OSKIT_EINVAL; /* XXX */
	}

	return 0;
}

static OSKIT_COMDECL_V
netdev_get_addr(oskit_etherdev_t *fdev,
	unsigned char out_addr[OSKIT_ETHERDEV_ADDR_SIZE])
{
	linuxnetdev_t *dev = (linuxnetdev_t *)(fdev-1);
	memcpy(out_addr, dev->ethaddr, OSKIT_ETHERDEV_ADDR_SIZE);
}

static OSKIT_COMDECL
netdev_rxpoll(oskit_etherdev_t *fdev, int *count, oskit_bufio_t *out_bufios[])
{
	return OSKIT_ENODEV;
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
	linuxnetdev_t *feth;
	int rc;

	feth = malloc(sizeof *feth);
	if (feth == 0)
		return OSKIT_ENOMEM;

	feth->nio.ops = &netio_ops;
	feth->eio.ops = &eth_ops;

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
static char default_devs[] = "eth1";

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
VERBOSE() { return rv; }
#endif
