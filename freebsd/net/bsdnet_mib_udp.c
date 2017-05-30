/*
 * Copyright (c) 2001 University of Utah and the Flux Group.
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
 * The FreeBSD networking stack's implementation of the `oskit_mib_udp' COM
 * interface.
 */

#include <oskit/mib.h>
#include <oskit/mib/udp.h>
#include <oskit/dev/freebsd.h>	/* For `oskit_freebsd_xlate_errno'. */

#include "bsdnet_mib_udp.h"

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/socketvar.h>	/* Needed to get `struct xinpgen'. */
#include <sys/sysctl.h>
#include <net/route.h>		/* Needed to include <netinet/in_pcb.h>. */
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_pcb.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>

/*
 * The following `SYSCTL' data items are defined in the OSKit file
 * `freebsd/3.x/src/sys/netinet/udp_usrreq.c'.
 */
/* extern int		udpcksum; */
/* extern int		log_in_vain; */
extern struct udpstat	udpstat;
extern int		udp_pcblist SYSCTL_HANDLER_ARGS;
/* extern int		udp_sendspace; */
/* extern int		udp_recvspace; */

/*
 * Types and macros for debugging our `sysctl'-style callback functions.
 */
#ifdef DEBUG
typedef enum { INIT, FIRST_XINPGEN, LAST_XINPGEN } env_state;
#  define DEBUG_CHECK_STATE(expr)	assert((expr))
#  define DEBUG_SET_STATE(expr)		(expr)
#else
#  define DEBUG_CHECK_STATE(expr)	/* Do nothing. */
#  define DEBUG_SET_STATE(expr)		/* Do nothing. */
#endif /* DEBUG */


/*****************************************************************************/

OSKIT_COMDECL
bsdnet_mib_udp_get_udpstats(
	oskit_mib_udp_t *				intf,
	/* OUT */ oskit_mib_slotset_udpstats_t *	ss)
{
	oskit_u32_t udp_in_errors
		= (udpstat.udps_hdrops + udpstat.udps_badsum
		   + udpstat.udps_badlen + udpstat.udps_fullsock);
	oskit_u32_t udp_in_datagrams
		= (udpstat.udps_ipackets
		   - udp_in_errors - udpstat.udps_noport);
	
	/*****/
	
	oskit_mib_u32_set(&(ss->udp_in_datagrams), udp_in_datagrams);
	oskit_mib_u32_set(&(ss->udp_no_ports), udpstat.udps_noport);
	oskit_mib_u32_set(&(ss->udp_in_errors), udp_in_errors);
	
	oskit_mib_u32_set(&(ss->udp_out_datagrams), udpstat.udps_opackets);
	
	return OSKIT_S_OK;
}


/*****************************************************************************/

/*
 * This is the type of the environment that is passed to the `get_one_udpentry'
 * callback function.
 */
typedef struct get_udpentry_env get_udpentry_env_t;
struct get_udpentry_env {
	/*
	 * The `sysctl' request data.  This must be the first struct member.
	 * The callback function receives a `struct sysctl_req *', and then
	 * casts it to be a pointer to this environment structure.
	 */
	struct sysctl_req	req;
	
	/*
	 * The arguments to `bsdnet_mib_udp_get_udpentry_table', which sets up
	 * the environment for the callback.
	 */
	struct {
		oskit_u32_t					start_row;
		oskit_u32_t					want_rows;
		oskit_mib_slotset_udpentry_t *			matching;
		/* OUT */ oskit_mib_slotset_udpentry_t *	table;
		/* OUT */ oskit_u32_t *				out_rows;
		/* OUT */ oskit_bool_t *			more_rows;
	} args;
	
	/*
	 * State that must be kept between invocations of the callback.
	 */
	struct xinpgen		xig;	/* The initial generator state. */
	oskit_bool_t		redo;	/* Do we need to rescan the list? */
	
#ifdef DEBUG
	env_state		state;
#endif
};

/*
 * Determine if the given (address, port) pair matches the given filter.
 */
static inline oskit_bool_t
udpentry_match(
	oskit_mib_in_addr_t		laddr,
	oskit_u16_t			lport,
	oskit_mib_slotset_udpentry_t *	filter)
{
	if (!filter)
		return 1;
	
	if (oskit_mib_mib_in_addr_test(&(filter->udp_local_address))
	    && (laddr
		!= oskit_mib_mib_in_addr_get(&(filter->udp_local_address))))
		return 0;
	if (oskit_mib_u16_test(&(filter->udp_local_port))
	    && (lport
		!= oskit_mib_u16_get(&(filter->udp_local_port))))
		return 0;
	
	return 1;
}

/*
 * This is the ``callback function'' that is invoked by `udp_pcblist' in order
 * to copy data out.  The data that is copied out is as follows:
 *
 *   + First, we get a `struct xinpgen' describing the current state of the
 *     generator.
 *   + Next, we get zero or more `struct xinpcb's, one for each UDP entry.
 *   + Finally, we get a `struct xinpgen' describing the (new) state of the
 *     generator.
 *
 * See the function `udp_pcblist' for further details.  Some of the code below
 * is modeled after code in the FreeBSD `netstat' utility; in FreeBSD, see the
 * file `/usr/src/usr.bin/netstat/inet.c'.
 */
static int
get_one_udpentry(
	struct sysctl_req *	req,
	const void *		buf,
	size_t			buf_size)
{
	get_udpentry_env_t *	env = (get_udpentry_env_t *) req;
	
	switch (buf_size) {
	default:
		/* This should never happen. */
		assert(!"Reached an unreachable code point.");
		break;
		
	case sizeof(struct xinpgen):
		assert(!env->redo);
		/*
		 * If `env->xin.xig_len' is zero, then we're doing the initial
		 * callback from function `udp_pcblist'.  Otherwise, we're
		 * doing the final callback.
		 */
		if (env->xig.xig_len == 0) {
			/* Copy the initial state structure. */
			DEBUG_CHECK_STATE((env->state == INIT));
			env->xig = *((struct xinpgen *) buf);
			DEBUG_SET_STATE((env->state = FIRST_XINPGEN));
		} else {
			/*
			 * Compare the final state to the initial state.  If
			 * the generation counters are different, then the set
			 * of PCBs has changed, and we need to rescan.
			 */
			struct xinpgen *final_xig = (struct xinpgen *) buf;
			
			DEBUG_CHECK_STATE((env->state == FIRST_XINPGEN));
			if (env->xig.xig_gen != final_xig->xig_gen)
				env->redo = 1;
			DEBUG_SET_STATE((env->state = LAST_XINPGEN));
		}
		break;
		
	case sizeof(struct xinpcb): {
		struct inpcb *		inp = &(((struct xinpcb *) buf)->
						xi_inp);
		struct xsocket *	so = &(((struct xinpcb *) buf)->
					       xi_socket);
		oskit_mib_in_addr_t	laddr = inp->inp_laddr.s_addr;
		oskit_u16_t		lport = inp->inp_lport;
		
		/* We must have already seen the initial `struct xinpgen'. */
		DEBUG_CHECK_STATE((env->state == FIRST_XINPGEN));
		
		/*
		 * Check that this PCB is valid.  It may have been modified
		 * after `udp_pcblist' decided to copy it out.  Note that the
		 * FreeBSD `netstat' program makes these same tests.
		 */
		if (so->xso_protocol != IPPROTO_UDP)
			break;
		if (inp->inp_gencnt > env->xig.xig_gen)
			break;
		
		if (udpentry_match(laddr, lport, env->args.matching)) {
			if (env->args.start_row > 0) {
				/* Skip this row. */
				env->args.start_row--;
			} else if (env->args.want_rows > 0) {
				/* Accumulate this row. */
				oskit_mib_mib_in_addr_set(
					&(env->args.table->udp_local_address),
					laddr);
				oskit_mib_u16_set(
					&(env->args.table->udp_local_port),
					lport);
				(*env->args.out_rows)++;
				env->args.table++;
				env->args.want_rows--;
				
			} else {
				/* Indicate that the table filled up. */
				*env->args.more_rows = 1;
			}
		}
		break;
	}
	}
	
	/* Indicate to the `udp_pcblist' function that no error occurred. */
	return 0;
}

/*
 * The implementation of the `get_udpentry_table' operation.
 */
OSKIT_COMDECL
bsdnet_mib_udp_get_udpentry_table(
	oskit_mib_udp_t *				intf,
	oskit_u32_t					start_row,
	oskit_u32_t					want_rows,
	oskit_mib_slotset_udpentry_t *			matching,
	/* OUT */ oskit_mib_slotset_udpentry_t *	table,
	/* OUT */ oskit_u32_t *				out_rows,
	/* OUT */ oskit_bool_t *			more_rows)
{
	get_udpentry_env_t	env;
	int			rc;
	
	/*
	 * This implementation works by calling FreeBSD's `sysctl' function for
	 * scanning the list of UDP entries.  We have to do a little work to
	 * call that function, since it expects a `sysctl'-style interface, but
	 * this actually works to our advantage.  Part of the interface is that
	 * the caller provides a function to ``copy out'' data.  Our copy-out
	 * function fills out the table.  Clever, eh?
	 */
	do {
		/*
		 * Set up the environment for the callback.
		 *
		 * Note that `env.req.oldptr' must be non-null.  This is part
		 * of the interface to `systcl'-style functions and tells the
		 * function that we're reading the current state.  Similarly,
		 * `env.req.newptr' must be null to tell the function that
		 * we're not writing new data.
		 */
		env.req.p		= 0;
		env.req.lock		= 0;
		env.req.oldptr		= &env;		/* Must be non-null. */
		env.req.oldlen		= sizeof(env);
		env.req.oldidx		= 0;
		env.req.oldfunc		= get_one_udpentry;
		env.req.newptr		= 0;		/* Must be null. */
		env.req.newlen		= 0;
		env.req.newidx		= 0;
		env.req.newfunc		= 0;
		env.args.start_row	= start_row;
		env.args.want_rows	= want_rows;
		env.args.matching	= matching;
		env.args.table		= table;
		env.args.out_rows	= out_rows;
		*(env.args.out_rows)	= 0;
		env.args.more_rows	= more_rows;
		*(env.args.more_rows)	= 0;
		env.xig.xig_len		= 0;
		/* env.xig...		other members left uninited. */
		env.redo		= 0;
		DEBUG_SET_STATE((env.state = INIT));
		
		/* The first three args to `udp_pcblist' are unused. */
		rc = udp_pcblist(0, 0, 0, &(env.req));
	} while (env.redo);
	
	DEBUG_CHECK_STATE((rc ? 1 : (env.state == LAST_XINPGEN)));
	
	return (rc ? oskit_freebsd_xlate_errno(rc) : OSKIT_S_OK);
}


/*****************************************************************************/

/* End of file. */

