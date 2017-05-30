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
 * Definition of the COM-based TCP MIB interface.
 */
#ifndef _OSKIT_MIB_TCP_H_
#define _OSKIT_MIB_TCP_H_

#include <oskit/com.h>
#include <oskit/mib.h>

/*
 * Struct communicated by `get_tcpstats'.
 */
typedef struct oskit_mib_slotset_tcpstats oskit_mib_slotset_tcpstats_t;
struct oskit_mib_slotset_tcpstats {
	/*
	 * These slots are pretty much verbatim from the SNMPv2 MIB-II `tcp'
	 * group, minus the tables.  For each slot, the SNMP type is shown.
	 * See IETF RFC 2012.
	 */
	
	/* INTEGER(1..4)   */ oskit_mib_u32_t	tcp_rto_algorithm;
	/* Integer32       */ oskit_mib_s32_t	tcp_rto_min;
	/* Integer32       */ oskit_mib_s32_t	tcp_rto_max;
	
	/* Integer32       */ oskit_mib_s32_t	tcp_max_conn;
	/* Counter32       */ oskit_mib_u32_t	tcp_active_opens;
	/* Counter32       */ oskit_mib_u32_t	tcp_passive_opens;
	/* Counter32       */ oskit_mib_u32_t	tcp_attempt_fails;
	/* Counter32       */ oskit_mib_u32_t	tcp_estab_resets;
	/* Gauge32         */ oskit_mib_u32_t	tcp_curr_estab;
	/* Counter32       */ oskit_mib_u32_t	tcp_in_segs;
	/* Counter32       */ oskit_mib_u32_t	tcp_out_segs;
	/* Counter32       */ oskit_mib_u32_t	tcp_retrans_segs;
	
	/* SEQUENCE OF TcpConnEntry		tcp_conn_table; */
	
	/* Counter32       */ oskit_mib_u32_t	tcp_in_errs;
	/* Counter32       */ oskit_mib_u32_t	tcp_out_rsts;
	
	/*********************************************************************/
	
	/*
	 * The following slots correspond to `SYSCTL'ed data in the FreeBSD 3.x
	 * networking stack that aren't made available through the slots above.
	 * Possibilities for future expansion.
	 */
	
	/* In `freebsd/3.x/src/sys/netinet/tcp_input.c':		*/
	/* oskit_mib_bool_t			log_in_vain;		*/
	/* oskit_mib_bool_t			tcp_delack_enabled;	*/
	
	/* In `freebsd/3.x/src/sys/netinet/tcp_subr.c':			*/
	/* oskit_mib_u32_t			tcp_mssdflt;		*/
	/* oskit_mib_u32_t			tcp_rttdflt;		*/
	/* oskit_mib_bool_t			tcp_do_rfc1323;		*/
	/* oskit_mib_bool_t			tcp_do_rfc1644;		*/
	/* oskit_mib_u32_t			tcbinfo.ipi_count;	*/
	
	/* In `freebsd/3.x/src/sys/netinet/tcp_timer.c':		*/
	/* oskit_mib_u32_t			tcp_keepinit;		*/
	/* oskit_mib_u32_t			tcp_keepidle;		*/
	/* oskit_mib_u32_t			tcp_keepintvl;		*/
	/* oskit_mib_bool_t			always_keepalive;	*/
	
	/* In `freebsd/3.x/src/sys/netinet/tcp_usrreq.c':		*/
	/* oskit_mib_u32_t			tcp_sendspace;		*/
	/* oskit_mib_u32_t			tcp_recvspace;		*/
	
	/*
	 * In `freebsd/3.x/src/sys/netinet/tcp_var.h': fields of `tcpstat' that
	 * aren't available, or aren't available individually.
	 */
	/* ...Too many to list here! */
};

/*
 * Struct communicated by `get_tcpconn_table'.
 */
typedef struct oskit_mib_slotset_tcpconn oskit_mib_slotset_tcpconn_t;
struct oskit_mib_slotset_tcpconn {
	/*
	 * These slots are pretty much verbatim from the SNMPv2 MIB-II `tcp'
	 * group's `tcpConnEntry' sequence.  For each slot, the SNMP type is
	 * shown.  See IETF RFC 2012.
	 */
	
	/* INTEGER(1..12)    */ oskit_mib_u32_t		tcp_conn_state;
	/* IpAddress         */ oskit_mib_mib_in_addr_t	tcp_conn_local_address;
	/* INTEGER(0..65535) */ oskit_mib_u16_t		tcp_conn_local_port;
	/* IpAddress         */ oskit_mib_mib_in_addr_t	tcp_conn_rem_address;
	/* INTEGER(0..65535) */ oskit_mib_u16_t		tcp_conn_rem_port;
};

/*
 * The values that a `tcp_conn_state' slot may have.  Each value corresponds to
 * a member of the `tcpConnState' enumeration defined in IETF RFC 2012.  The
 * encoding is the same as well; i.e., there's no need to translate between the
 * SNMP and OSKIT encodings of the TCP connections states.
 */
#define OSKIT_MIB_TCP_CONN_STATE_CLOSED		(1)
#define OSKIT_MIB_TCP_CONN_STATE_LISTEN		(2)
#define OSKIT_MIB_TCP_CONN_STATE_SYN_SENT	(3)
#define OSKIT_MIB_TCP_CONN_STATE_SYN_RECEIVED	(4)
#define OSKIT_MIB_TCP_CONN_STATE_ESTABLISHED	(5)
#define OSKIT_MIB_TCP_CONN_STATE_FIN_WAIT_1	(6)
#define OSKIT_MIB_TCP_CONN_STATE_FIN_WAIT_2	(7)
#define OSKIT_MIB_TCP_CONN_STATE_CLOSE_WAIT	(8)
#define OSKIT_MIB_TCP_CONN_STATE_LAST_ACK	(9)
#define OSKIT_MIB_TCP_CONN_STATE_CLOSING	(10)
#define OSKIT_MIB_TCP_CONN_STATE_TIME_WAIT	(11)
#define OSKIT_MIB_TCP_CONN_STATE_DELETE_TCB	(12)

/*
 * TCP MIB interface.
 * IID 4aa7e005-7c74-11cf-b500-08000953adc2
 */
struct oskit_mib_tcp {
	struct oskit_mib_tcp_ops *ops;
};
typedef struct oskit_mib_tcp oskit_mib_tcp_t;

struct oskit_mib_tcp_ops {
	/*
	 * COM-specified IUnknown interface operations.
	 */
	OSKIT_COMDECL_IUNKNOWN(oskit_mib_tcp_t)

	/*
	 * Operations specific to the TCP monitor interface.
	 *
	 * In the SNMPv2 MIB-II `tcp' group, all slots are read-only (except
	 * for the `tcpConnState' of each entry in the `tcpConnTable', but we
	 * don't put the table in our `tcpstats' structure).
	 */
	OSKIT_MIB_SLOTSET_OPS_RO	(oskit_mib_tcp_t,
					 oskit_mib_slotset_tcpstats_t,
					 tcpstats)
	OSKIT_MIB_SLOTSET_TABLE_OPS_RW	(oskit_mib_tcp_t,
					 oskit_mib_slotset_tcpconn_t,
					 tcpconn)
};

#define oskit_mib_tcp_query(tcp, iid, out_ihandle) \
	((tcp)->ops->query((oskit_mib_tcp_t *)(tcp), (iid), (out_ihandle)))
#define oskit_mib_tcp_addref(tcp) \
	((tcp)->ops->addref((oskit_mib_tcp_t *)(tcp)))
#define oskit_mib_tcp_release(tcp) \
	((tcp)->ops->release((oskit_mib_tcp_t *)(tcp)))

#define oskit_mib_tcp_get_tcpstats(tcp, tcpstats) \
	((tcp)->ops->get_tcpstats((oskit_mib_tcp_t *)(tcp), (tcpstats)))

#define oskit_mib_tcp_get_tcpconn_table(tcp,				\
					start_row, want_rows, matching,	\
					table, out_rows, more_rows)	\
	((tcp)->ops->get_tcpconn_table((oskit_mib_tcp_t *)(tcp),	\
				       (start_row),			\
				       (want_rows),			\
				       (matching),			\
				       /* OUT */ (table),		\
				       /* OUT */ (out_rows),		\
				       /* OUT */ (more_rows)))
#define oskit_mib_tcp_set_tcpconn_table(tcp,				\
					matching, all_matches, vals)	\
	((tcp)->ops->get_tcpconn_table((oskit_mib_tcp_t *)(tcp),	\
				       (matching),			\
				       (all_matches),			\
				       (vals)))

/* GUID for oskit_mib_tcp interface */
extern const struct oskit_guid oskit_mib_tcp_iid;
#define OSKIT_MIB_TCP_IID OSKIT_GUID(0x4aa7e005, 0x7c74, 0x11cf, \
				0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#endif /* _OSKIT_MIB_TCP_H_ */

/* End of file. */

