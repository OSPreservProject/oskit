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
 * The FreeBSD networking stack's implementation of the `oskit_mib_tcp' COM
 * interface.
 */

#include <oskit/mib.h>
#include <oskit/mib/tcp.h>
#include <oskit/dev/freebsd.h>	/* For `oskit_freebsd_xlate_errno'. */

#include "bsdnet_mib_tcp.h"

#include <sys/param.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/sysctl.h>
#include <net/route.h>		/* Needed to include <netinet/in_pcb.h>. */
#include <netinet/in.h>
#include <netinet/in_pcb.h>
#include <netinet/tcp.h>
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>

/*
 * The following `SYSCTL' data items are defined in the OSKit file
 * `freebsd/3.x/src/sys/netinet/tcp_input.c'.
 */
extern struct tcpstat	tcpstat;
/* extern int		log_in_vain; */
/* extern int		tcp_delack_enabled; */

/*
 * The following `SYSCTL' data items are defined in the OSKit file
 * `freebsd/3.x/src/sys/netinet/tcp_subr.c'.
 */
/* extern int		tcp_mssdflt; */
/* extern int		tcp_rttdflt; */
/* extern int		tcp_do_rfc1323; */
/* extern int		tcp_do_rfc1644; */
/* extern int		tcbinfo.ipi_count; */
extern int		tcp_pcblist SYSCTL_HANDLER_ARGS;

/*
 * The following `SYSCTL' data items are defined in the OSKit file
 * `freebsd/3.x/src/sys/netinet/tcp_timer.c'.
 */
/* extern int		tcp_keepinit; */
/* extern int		tcp_keepidle; */
/* extern int		tcp_keepintvl; */
/* extern int		always_keepalive; */

/*
 * The following `SYSCTL' data items are defined in the OSKit file
 * `freebsd/3.x/src/sys/netinet/tcp_usrreq.c'.
 */
/* extern int		tcp_sendspace; */
/* extern int		tcp_recvspace; */

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

/*
 * This is the type of the environment that is passed to the
 * `count_one_tcpconn' callback function.
 */
typedef struct count_tcpconn_env count_tcpconn_env_t;
struct count_tcpconn_env {
	/*
	 * The `sysctl' request data.  This must be the first struct member.
	 * The callback function receives a `struct sysctl_req *', and then
	 * casts it to be a pointer to this environment structure.
	 */
	struct sysctl_req	req;
	
	/*
	 * State that must be kept between invocations of the callback.
	 */
	struct xinpgen		xig;	/* The initial generator state. */
	oskit_u32_t		estab;	/* # of estab'ed & close-wait conns. */
	oskit_bool_t		redo;	/* Do we need to rescan the list? */
#ifdef DEBUG
	env_state		state;
#endif
};

/*
 * This is the ``callback function'' that is invoked by `tcp_pcblist' in order
 * to copy data out for our `count_tcp_curr_estab' function.  The data that we
 * receive from `tcp_pcblist' is as follows:
 *
 *   + First, we get a `struct xinpgen' describing the current state of the
 *     generator.
 *   + Next, we get zero or more `struct xtcpcb's, one for each TCP entry.
 *   + Finally, we get a `struct xinpgen' describing the (new) state of the
 *     generator.
 *
 * See the function `tcp_pcblist' for further details.  Some of the code below
 * is modeled after code in the FreeBSD `netstat' utility; in FreeBSD, see the
 * file `/usr/src/usr.bin/netstat/inet.c'.
 */
static int
count_one_tcpconn(
	struct sysctl_req *	req,
	const void *		buf,
	size_t			buf_size)
{
	count_tcpconn_env_t *	env = (count_tcpconn_env_t *) req;
	
	switch (buf_size) {
	default:
		/* This should never happen. */
		assert(!"Reached an unreachable code point.");
		break;
		
	case sizeof(struct xinpgen):
		assert(!env->redo);
		/*
		 * If `env->xin.xig_len' is zero, then we're doing the initial
		 * callback from function `tcp_pcblist'.  Otherwise, we're
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
		
	case sizeof(struct xtcpcb): {
		struct tcpcb *		tp = &(((struct xtcpcb *) buf)->
					       xt_tp);
		struct inpcb *		inp = &(((struct xtcpcb *) buf)->
						xt_inp);
		struct xsocket *	so = &(((struct xtcpcb *) buf)->
					       xt_socket);
		
		/* We must have already seen the initial `struct xinpgen'. */
		DEBUG_CHECK_STATE((env->state == FIRST_XINPGEN));
		
		/*
		 * Check that this PCB is valid.  It may have been modified
		 * after `tcp_pcblist' decided to copy it out.  Note that the
		 * FreeBSD `netstat' program makes these same tests.
		 */
		if (so->xso_protocol != IPPROTO_TCP)
			break;
		if (inp->inp_gencnt > env->xig.xig_gen)
			break;
		
		if ((tp->t_state == TCPS_ESTABLISHED)
		    || (tp->t_state == TCPS_CLOSE_WAIT))
			++(env->estab);
		break;
	}
	}
	
	/* Indicate to the `tcp_pcblist' function that no error occurred. */
	return 0;
}

/*
 * Count the number of TCP connections for which the current state is either
 * `ESTABLISHED' or `CLOSE-WAIT'.
 */
static oskit_error_t
count_tcp_curr_estab(oskit_u32_t *estab)
{
	count_tcpconn_env_t	env;
	int			rc;
	
	/*
	 * This implementation works by calling FreeBSD's `sysctl' function for
	 * scanning the list of TCP entries.  We have to do a little work to
	 * call that function, since it expects a `sysctl'-style interface, but
	 * this actually works to our advantage.  Part of the interface is that
	 * the caller provides a function to ``copy out'' data.  Our copy-out
	 * function updates out counter.  Clever, eh?
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
		env.req.oldfunc		= count_one_tcpconn;
		env.req.newptr		= 0;		/* Must be null. */
		env.req.newlen		= 0;
		env.req.newidx		= 0;
		env.req.newfunc		= 0;
		env.xig.xig_len		= 0;
		/* env.xig...		other members left uninited. */
		env.estab		= 0;
		env.redo		= 0;
		DEBUG_SET_STATE((env.state = INIT));
		
		/* The first three args to `tcp_pcblist' are unused. */
		rc = tcp_pcblist(0, 0, 0, &(env.req));
	} while (env.redo);
	
	DEBUG_CHECK_STATE((rc ? 1 : (env.state == LAST_XINPGEN)));
	
	*estab = env.estab;
	
	return (rc ? oskit_freebsd_xlate_errno(rc) : OSKIT_S_OK);
}

/*
 * The implementation of the `get_tcpstats' operation.
 */
OSKIT_COMDECL
bsdnet_mib_tcp_get_tcpstats(
	oskit_mib_tcp_t *				intf,
	/* OUT */ oskit_mib_slotset_tcpstats_t *	ss)
{
	int rc;
	oskit_u32_t tcp_curr_estab;
	oskit_u32_t tcp_out_segs
		= (tcpstat.tcps_sndtotal - tcpstat.tcps_sndrexmitpack);
	oskit_u32_t tcp_in_errs
		= (tcpstat.tcps_rcvbadsum + tcpstat.tcps_rcvbadoff
		   + tcpstat.tcps_rcvshort);
	oskit_u32_t tcp_out_rsts
		= (tcpstat.tcps_sndctrl - tcpstat.tcps_closed);
	
	/*****/
	
	rc = count_tcp_curr_estab(&tcp_curr_estab);
	
	oskit_mib_u32_set(&(ss->tcp_rto_algorithm), 4); /* Van Jacobson */
	/*
	 * SNMP: `tcp_rto_min' and `tcp_rto_max' are measured in milliseconds.
	 */
	oskit_mib_s32_set(&(ss->tcp_rto_min), (TCPTV_MIN / PR_SLOWHZ
					       * 1000));
	oskit_mib_s32_set(&(ss->tcp_rto_max), (TCPTV_REXMTMAX / PR_SLOWHZ
					       * 1000));
	
	/*
	 * SNMP: ``In entities where the maximum number of connections is
	 * dynamic, this object [`tcp_max_conn'] should contain the value -1.''
	 */
	oskit_mib_s32_set(&(ss->tcp_max_conn), -1);
	oskit_mib_u32_set(&(ss->tcp_active_opens), tcpstat.tcps_connattempt);
	oskit_mib_u32_set(&(ss->tcp_passive_opens), tcpstat.tcps_accepts);
	/*
	 * The UCD-SNMP 4.1.1 code has XXX's by its implementation of the next
	 * two items, and their implementation is the same as ours.
	 *
	 * After looking at the FreeBSD networking code, I think they're as
	 * right as we can get.  (The only ``wrong thing'': when the FreeBSD
	 * stack counts drops a connection that is in the SYN-RECVD state, it
	 * increments the `tcps_drops' counter.  But the TCP MIB expects one to
	 * report this event in the `tcpAttemptFails' counter.)
	 */
	oskit_mib_u32_set(&(ss->tcp_attempt_fails), tcpstat.tcps_conndrops);
	oskit_mib_u32_set(&(ss->tcp_estab_resets), tcpstat.tcps_drops);
	if (rc == OSKIT_S_OK)
		oskit_mib_u32_set(&(ss->tcp_curr_estab), tcp_curr_estab);
	else
		oskit_mib_u32_clear(&(ss->tcp_curr_estab));
	oskit_mib_u32_set(&(ss->tcp_in_segs), tcpstat.tcps_rcvtotal);
	oskit_mib_u32_set(&(ss->tcp_out_segs), tcp_out_segs);
	oskit_mib_u32_set(&(ss->tcp_retrans_segs), tcpstat.tcps_sndrexmitpack);
	
	oskit_mib_u32_set(&(ss->tcp_in_errs), tcp_in_errs);
	oskit_mib_u32_set(&(ss->tcp_out_rsts), tcp_out_rsts);
	
	return OSKIT_S_OK;
}


/*****************************************************************************/

/*
 * This is the type of the environment that is passed to the `get_one_tcpconn'
 * callback function.
 */
typedef struct get_tcpconn_env get_tcpconn_env_t;
struct get_tcpconn_env {
	/*
	 * The `sysctl' request data.  This must be the first struct member.
	 * The callback function receives a `struct sysctl_req *', and then
	 * casts it to be a pointer to this environment structure.
	 */
	struct sysctl_req	req;
	
	/*
	 * The arguments to `bsdnet_mib_tcp_get_tcpconn_table', which sets up
	 * the environment for the callback.
	 */
	struct {
		oskit_u32_t					start_row;
		oskit_u32_t					want_rows;
		oskit_mib_slotset_tcpconn_t *			matching;
		/* OUT */ oskit_mib_slotset_tcpconn_t *		table;
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
 * Determine if the information about the given TCP connection (`tcp_conn_*')
 * matches the given filter.
 */
static inline oskit_bool_t
tcpconn_match(
	oskit_u32_t			tcp_conn_state,
	oskit_mib_in_addr_t		tcp_conn_local_address,
	oskit_u16_t			tcp_conn_local_port,
	oskit_mib_in_addr_t		tcp_conn_rem_address,
	oskit_u16_t			tcp_conn_rem_port,
	oskit_mib_slotset_tcpconn_t *	filter)
{
	if (!filter)
		return 1;
	
#define MATCH_SLOT(slot_type, slot_name)				\
	if (oskit_mib_##slot_type##_test(&(filter->slot_name))		\
	    && (slot_name						\
		!= oskit_mib_##slot_type##_get(&(filter->slot_name))))	\
		return 0
	
	MATCH_SLOT(u32,		tcp_conn_state);
	MATCH_SLOT(mib_in_addr,	tcp_conn_local_address);
	MATCH_SLOT(u16,		tcp_conn_local_port);
	MATCH_SLOT(mib_in_addr,	tcp_conn_rem_address);
	MATCH_SLOT(u16,		tcp_conn_rem_port);
#undef MATCH_SLOT
	
	return 1;
}

/*
 * Translate from a FreeBSD TCP connection state value (`t_state') to an OSKit
 * MIB TCP connection state value.
 */
static inline oskit_u32_t
xlate_tcpconn_state(int t_state)
{
	switch (t_state) {
#define XLATE_STATE(state)						\
	case TCPS_##state: return OSKIT_MIB_TCP_CONN_STATE_##state
	
	XLATE_STATE(CLOSED);
	XLATE_STATE(LISTEN);
	XLATE_STATE(SYN_SENT);
	XLATE_STATE(SYN_RECEIVED);
	XLATE_STATE(ESTABLISHED);
	XLATE_STATE(CLOSE_WAIT);
	XLATE_STATE(FIN_WAIT_1);
	XLATE_STATE(CLOSING);
	XLATE_STATE(LAST_ACK);
	XLATE_STATE(FIN_WAIT_2);
	XLATE_STATE(TIME_WAIT);
#undef XLATE_STATE
	default:
		/* This should never happen. */
	}
	
	/* This should never happen. */
	return OSKIT_MIB_TCP_CONN_STATE_ESTABLISHED;
}

/*
 * This is the ``callback function'' that is invoked by `tcp_pcblist' in order
 * to copy data out for our `bsdnet_mib_tcp_get_tcpconn_table' function.  The
 * data that we receive from `tcp_pcblist' is as follows:
 *
 *   + First, we get a `struct xinpgen' describing the current state of the
 *     generator.
 *   + Next, we get zero or more `struct xtcpcb's, one for each TCP entry.
 *   + Finally, we get a `struct xinpgen' describing the (new) state of the
 *     generator.
 *
 * See the function `tcp_pcblist' for further details.  Some of the code below
 * is modeled after code in the FreeBSD `netstat' utility; in FreeBSD, see the
 * file `/usr/src/usr.bin/netstat/inet.c'.
 */
static int
get_one_tcpconn(
	struct sysctl_req *	req,
	const void *		buf,
	size_t			buf_size)
{
	get_tcpconn_env_t *	env = (get_tcpconn_env_t *) req;
	
	switch (buf_size) {
	default:
		/* This should never happen. */
		assert(!"Reached an unreachable code point.");
		break;
		
	case sizeof(struct xinpgen):
		assert(!env->redo);
		/*
		 * If `env->xin.xig_len' is zero, then we're doing the initial
		 * callback from function `tcp_pcblist'.  Otherwise, we're
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
		
	case sizeof(struct xtcpcb): {
		struct tcpcb *		tp = &(((struct xtcpcb *) buf)->
					       xt_tp);
		struct inpcb *		inp = &(((struct xtcpcb *) buf)->
						xt_inp);
		struct xsocket *	so = &(((struct xtcpcb *) buf)->
					       xt_socket);
		
		oskit_u32_t		tcp_conn_state;
		oskit_mib_in_addr_t	tcp_conn_local_address;
		oskit_u16_t		tcp_conn_local_port;
		oskit_mib_in_addr_t	tcp_conn_rem_address;
		oskit_u16_t		tcp_conn_rem_port;
		
		/* We must have already seen the initial `struct xinpgen'. */
		DEBUG_CHECK_STATE((env->state == FIRST_XINPGEN));
		
		/*
		 * Check that this PCB is valid.  It may have been modified
		 * after `tcp_pcblist' decided to copy it out.  Note that the
		 * FreeBSD `netstat' program makes these same tests.
		 */
		if (so->xso_protocol != IPPROTO_TCP)
			break;
		if (inp->inp_gencnt > env->xig.xig_gen)
			break;
		
		/*
		 * We copy for convenience, not out of necessity.  `inp' isn't
		 * a live kernel data structure --- see `tcp_pcblist'.
		 */
		tcp_conn_state		= xlate_tcpconn_state(tp->t_state);
		tcp_conn_local_address	= inp->inp_laddr.s_addr;
		tcp_conn_local_port	= inp->inp_lport;
		tcp_conn_rem_address	= inp->inp_faddr.s_addr;
		tcp_conn_rem_port	= inp->inp_fport;
		
		if (tcpconn_match(tcp_conn_state,
				  tcp_conn_local_address, tcp_conn_local_port,
				  tcp_conn_rem_address, tcp_conn_rem_port,
				  env->args.matching)) {
			if (env->args.start_row > 0) {
				/* Skip this row. */
				env->args.start_row--;
			} else if (env->args.want_rows > 0) {
				/* Accumulate this row. */
#define COPY_OUT(slot_type, slot_name)					\
	oskit_mib_##slot_type##_set(&(env->args.table->slot_name),	\
				    slot_name)
				COPY_OUT(u32, tcp_conn_state);
				COPY_OUT(mib_in_addr, tcp_conn_local_address);
				COPY_OUT(u16, tcp_conn_local_port);
				COPY_OUT(mib_in_addr, tcp_conn_rem_address);
				COPY_OUT(u16, tcp_conn_rem_port);
#undef COPY_OUT
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
	
	/* Indicate to the `tcp_pcblist' function that no error occurred. */
	return 0;
}

/*
 * The implementation of the `get_tcpconn_table' operation.
 */
OSKIT_COMDECL
bsdnet_mib_tcp_get_tcpconn_table(
	oskit_mib_tcp_t *				intf,
	oskit_u32_t					start_row,
	oskit_u32_t					want_rows,
	oskit_mib_slotset_tcpconn_t *			matching,
	/* OUT */ oskit_mib_slotset_tcpconn_t *		table,
	/* OUT */ oskit_u32_t *				out_rows,
	/* OUT */ oskit_bool_t *			more_rows)
{
	get_tcpconn_env_t	env;
	int			rc;
	
	/*
	 * This implementation works by calling FreeBSD's `sysctl' function for
	 * scanning the list of TCP entries.  We have to do a little work to
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
		env.req.oldfunc		= get_one_tcpconn;
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
		
		/* The first three args to `tcp_pcblist' are unused. */
		rc = tcp_pcblist(0, 0, 0, &(env.req));
	} while (env.redo);
	
	DEBUG_CHECK_STATE((rc ? 1 : (env.state == LAST_XINPGEN)));
	
	return (rc ? oskit_freebsd_xlate_errno(rc) : OSKIT_S_OK);
}


/*****************************************************************************/

/*
 * This is the type of the environment that is passed to the `set_one_tcpconn'
 * callback function.
 */
typedef struct set_tcpconn_env set_tcpconn_env_t;
struct set_tcpconn_env {
	/*
	 * The `sysctl' request data.  This must be the first struct member.
	 * The callback function receives a `struct sysctl_req *', and then
	 * casts it to be a pointer to this environment structure.
	 */
	struct sysctl_req	req;
	
	/*
	 * The arguments to `bsdnet_mib_tcp_set_tcpconn_table', which sets up
	 * the environment for the callback.
	 */
	struct {
		oskit_mib_slotset_tcpconn_t *			matching;
		oskit_bool_t					all_matches;
		oskit_mib_slotset_tcpconn_t *			ss;
	} args;
	
	/*
	 * State that must be kept between invocations of the callback.
	 */
	struct xinpgen		xig;	/* The initial generator state. */
	oskit_bool_t		set_one;/* Have we mod'ed 1+ connections? */
	oskit_bool_t		redo;	/* Do we need to rescan the list? */
	
#ifdef DEBUG
	env_state		state;
#endif
};

/*
 * This is the ``callback function'' that is invoked by `tcp_pcblist' in order
 * to copy data out for our `bsdnet_mib_tcp_set_tcpconn_table' function.  This
 * callback is quite similar to `get_one_tcpconn'; see the comments for that
 * function for more information.
 */
static int
set_one_tcpconn(
	struct sysctl_req *	req,
	const void *		buf,
	size_t			buf_size)
{
	set_tcpconn_env_t *	env = (set_tcpconn_env_t *) req;
	
	switch (buf_size) {
	default:
		/* This should never happen. */
		assert(!"Reached an unreachable code point.");
		break;
		
	case sizeof(struct xinpgen):
		assert(!env->redo);
		/*
		 * If `env->xin.xig_len' is zero, then we're doing the initial
		 * callback from function `tcp_pcblist'.  Otherwise, we're
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
		
	case sizeof(struct xtcpcb): {
		struct tcpcb *		tp = &(((struct xtcpcb *) buf)->
					       xt_tp);
		struct inpcb *		inp = &(((struct xtcpcb *) buf)->
						xt_inp);
		struct xsocket *	so = &(((struct xtcpcb *) buf)->
					       xt_socket);
		
		oskit_u32_t		tcp_conn_state;
		oskit_mib_in_addr_t	tcp_conn_local_address;
		oskit_u16_t		tcp_conn_local_port;
		oskit_mib_in_addr_t	tcp_conn_rem_address;
		oskit_u16_t		tcp_conn_rem_port;
		
		/* We must have already seen the initial `struct xinpgen'. */
		DEBUG_CHECK_STATE((env->state == FIRST_XINPGEN));
		
		/* Bail out early if we've already done all our work. */
		if (env->set_one && !env->args.all_matches)
			break;
		
		/*
		 * Check that this PCB is valid.  It may have been modified
		 * after `tcp_pcblist' decided to copy it out.  Note that the
		 * FreeBSD `netstat' program makes these same tests.
		 */
		if (so->xso_protocol != IPPROTO_TCP)
			break;
		if (inp->inp_gencnt > env->xig.xig_gen)
			break;
		
		/*
		 * We copy for convenience, not out of necessity.  `inp' isn't
		 * a live kernel data structure --- see `tcp_pcblist'.
		 */
		tcp_conn_state		= xlate_tcpconn_state(tp->t_state);
		tcp_conn_local_address	= inp->inp_laddr.s_addr;
		tcp_conn_local_port	= inp->inp_lport;
		tcp_conn_rem_address	= inp->inp_faddr.s_addr;
		tcp_conn_rem_port	= inp->inp_fport;
		
		if (tcpconn_match(tcp_conn_state,
				  tcp_conn_local_address, tcp_conn_local_port,
				  tcp_conn_rem_address, tcp_conn_rem_port,
				  env->args.matching)) {
			/*
			 * The only request we can actually service is to kill
			 * the connection.  `bsdnet_mib_tcp_set_tcpconn_table'
			 * checks that this is in fact the requested action.
			 */
			tcp_usrreqs.pru_abort(so->xso_so);
			env->set_one = 1;
		}
		break;
	}
	}
	
	/* Indicate to the `tcp_pcblist' function that no error occurred. */
	return 0;
}

/*
 * The implementation of the `set_tcpconn_table' operation.
 */
OSKIT_COMDECL
bsdnet_mib_tcp_set_tcpconn_table(
	oskit_mib_tcp_t *				intf,
	oskit_mib_slotset_tcpconn_t *			matching,
	oskit_bool_t					all_matches,
	oskit_mib_slotset_tcpconn_t *			ss)
{
	set_tcpconn_env_t	env;
	int			rc;
	
	/*
	 * The only settable slot in a `oskit_mib_slotset_tcpconn_t' is the
	 * connection state, and the only value that may be set is
	 * `OSKIT_MIB_TCP_CONN_STATE_DELETE_TCB'.  In other words, the only
	 * serviceable request is to kill off a set of connections.
	 */
	if (oskit_mib_mib_in_addr_test(&(ss->tcp_conn_local_address)))
		return OSKIT_EPERM;
	if (oskit_mib_u16_test(&(ss->tcp_conn_local_port)))
		return OSKIT_EPERM;
	if (oskit_mib_mib_in_addr_test(&(ss->tcp_conn_rem_address)))
		return OSKIT_EPERM;
	if (oskit_mib_u16_test(&(ss->tcp_conn_rem_port)))
		return OSKIT_EPERM;
	if (!oskit_mib_u32_test(&(ss->tcp_conn_state)))
		/* User doesn't want to set the state?  We're done, I guess. */
		return OSKIT_S_OK;
	if (oskit_mib_u32_get(&(ss->tcp_conn_state))
	    != OSKIT_MIB_TCP_CONN_STATE_DELETE_TCB)
		return OSKIT_EPERM;
	
	do {
		/*
		 * Set up the environment for the callback.
		 */
		env.req.p		= 0;
		env.req.lock		= 0;
		env.req.oldptr		= &env;		/* Must be non-null. */
		env.req.oldlen		= sizeof(env);
		env.req.oldidx		= 0;
		env.req.oldfunc		= set_one_tcpconn;
		env.req.newptr		= 0;		/* Must be null. */
		env.req.newlen		= 0;
		env.req.newidx		= 0;
		env.req.newfunc		= 0;
		env.args.matching	= matching;
		env.args.all_matches	= all_matches;
		env.args.ss		= ss;
		env.xig.xig_len		= 0;
		/* env.xig...		other members left uninited. */
		env.set_one		= 0;
		env.redo		= 0;
		DEBUG_SET_STATE((env.state = INIT));
		
		/* The first three args to `tcp_pcblist' are unused. */
		rc = tcp_pcblist(0, 0, 0, &(env.req));
		/*
		 * If `all_matches' is false and we set at least one row, don't
		 * redo.
		 */
		if (!all_matches && env.set_one)
			env.redo = 0;
	} while (env.redo);
	
	DEBUG_CHECK_STATE((rc ? 1 : (env.state == LAST_XINPGEN)));
	
	return (rc ? oskit_freebsd_xlate_errno(rc) : OSKIT_S_OK);
}


/*****************************************************************************/

/* End of file. */

