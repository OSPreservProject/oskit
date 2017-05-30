/*
 * Copyright (c) 1996-2001 University of Utah and the Flux Group.
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
 * Routines to initialize the freebsd_net code.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/domain.h>
#include <sys/sockio.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include "net/netisr.h"
#include "net/if.h"
#include "net/route.h"
#include "netinet/in.h"
#include "netinet/if_ether.h"
#include "netinet/in_systm.h"
#include "netinet/ip.h"
#include "netinet/ip_var.h"

#include <oskit/net/socket.h>
#include "bsdnet_com.h"
#include "bsdnet_net_io.h"

#include "glue.h"

#if 0
extern void arpintr(void);
extern void ipintr(void);

/* this is what NETISR_SET(NETISR_ARP, arpintr); does */
struct netisrtab mn_arp = { NETISR_ARP, arpintr };
struct netisrtab mn_ip = { NETISR_IP, ipintr };

struct faked_linker_set {
	int	ls_length;
	caddr_t	ls_items[200]; 
} netisr_set = { 2, 
	{ (caddr_t)&mn_arp, 
	  (caddr_t)&mn_ip, 
	  (caddr_t)0 }
};
#else
extern void freebsd_netisr_arpintr_init(void);
extern void freebsd_netisr_ipintr_init(void);

extern struct linker_set netisr_set;
#endif

#if 0
extern struct domain routedomain;
extern struct domain inetdomain;

/* comes from DOMAIN_SET(route); */
struct faked_linker_set2 {
	int	ls_length;
	caddr_t	ls_items[200]; 
} domain_set = { 2, 
	{ (caddr_t)&routedomain, 
	  (caddr_t)&inetdomain, 
	  (caddr_t)0 }
};
#else
extern void freebsd_routedomain_init(void);
extern void freebsd_inetdomain_init(void);

extern struct linker_set domain_set;
#endif


/* array of software interrupts */
void    (*netisrs[32]) (void);
static void setup_netisrs(struct linker_set *);

extern void ifinit (void *dummy);
extern void domaininit (void *dummy);
extern void mbinit (void *dummy);
extern void loopattach (void *dummy);


void
initialize_netisr_linker_set() 
{
	freebsd_netisr_arpintr_init();
	freebsd_netisr_ipintr_init();
}

void
initialize_domain_linker_set() 
{
	freebsd_routedomain_init();
	freebsd_inetdomain_init();
}


/*
 * initialize the TCP/IP stack
 *
 * assume clock and other stuff is already initialized
 */
#ifdef KNIT
/* renamed to avoid conflict with prototype */
oskit_error_t 
oskit_freebsd_net_init2(oskit_socket_factory_t **f)
#else
oskit_error_t 
oskit_freebsd_net_init(oskit_services_t *osenv, oskit_socket_factory_t **f)
#endif
{
	struct proc p;

	OSKIT_FREEBSD_CREATE_CURPROC(p);
#ifdef INDIRECT_OSENV
	freebsd_oskit_osenv_init(osenv);
#endif
	oskit_freebsd_init();



	initialize_netisr_linker_set();
	initialize_domain_linker_set();

        /*
         * Quickly wire in netisrs.
         */
        setup_netisrs((struct linker_set *)&netisr_set);

        /* Initialize mbuf's and protocols. */
        ifinit(0);
	mbinit(0);
        domaininit(0);

	loopattach(0);

	OSKIT_FREEBSD_DESTROY_CURPROC(p);

	/*
	 * register our socket factory
	 */
	*f = &(oskit_freebsd_net_impl.sf);

	return 0;
}


/* ---------------------------------------------------------------------- */
/* Taken from sys/i386/i386/machdep.c: */
static void
setup_netisrs(struct linker_set *ls)
{
        int i;
        const struct netisrtab *nit;

        for(i = 0; ls->ls_items[i]; i++) {
                nit = (const struct netisrtab *)ls->ls_items[i];
                netisrs[nit->nit_num] = nit->nit_isr;
        }
}

