/*
 * Copyright (c) 2001, 2002 University of Utah and the Flux Group.
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
#include <stdarg.h>

/*
 * Emulab.net Testbed Master Control Protocol (TMCP) functions.
 */

#define TMCP_HOST	"155.101.128.70"	/* boss.emulab.net */
#define TMCP_PORT	7777

/*
 * Contact the Testbed Master Control Daemon (TMCD) and collect hostname
 * and interface information about this node.  The node uses BOOTP (via
 * the bootp library) to discover its identity.  The interface over which
 * the BOOTP response is received is assumed to be the "control net" where
 * TMCP_HOST resides.
 *
 * start_tmcp should be called in lieu of start_network.  It attempts to
 * open all interfaces to find the control net so the control net interface
 * should not be open when called.  Returns with all interfaces closed.
 *
 * If waitforall is non-zero, start_tmcp will repeatedly query TMCD until
 * all other nodes in the experiment have reported in.
 *
 * Must be called prior to any other tmcp_* routine.
 */
int start_tmcp(int waitforall);

/*
 * Returns TMCP provided IP address/mask information for a specific
 * OSKIT interface.  If return value is zero, ASCII strings in standard
 * dot notation are returned in the indicated pointers.  This routine will
 * allocate the memory needed for the strings with malloc, it is up to the
 * caller to free that memory.
 */
int start_tmcp_getinfo_if(int oskitifn, char **addrp, char **maskp);

/*
 * Returns TMCP (actually BOOTP) provided host information for the node:
 * the host name, DNS domain, a default gateway IP address, and a list
 * of nameserver IP addresses.  If return value is zero, ASCII strings
 * are returned in the indicated pointers.  This routine will allocate
 * the memory needed for the strings with malloc, it is up to the caller
 * to free that memory.
 *
 * Note that nsnamesp is an array of pointers with the final pointer NULL.
 * The caller must deallocate both the strings and the array.
 */
int start_tmcp_getinfo_host(char **hnamep, char **dnamep, char **gwnamep,
			    char ***nsnamesp);

/*
 * Using TMCP provided info, map the provided host name (sans domain)
 * into an IP address.  If localonly is non-zero it looks for the host
 * name exactly and returns a directly reachable address.  Otherwise it
 * looks for an alias for the host on another network in the experiment
 * (i.e., reachable via caller provided routing).  If the routine returns
 * zero, the address is returned in *addr.  Non-zero is returned if the
 * hostname didn't match any TMCP provided info.
 */
struct in_addr;
int tmcp_name2addr(char *host, struct in_addr *addr, int localonly);

/*
 * Using TMCP provided info, map the provided IP address to an appropriate
 * interface (taking into account the netmask for the interface).
 * *oskitifn is non-zero on call, that value is returned as a default if
 * no interfaces were configured with TMCP.  If the routine returns zero,
 * *oskitifn contains the OSKit interface number (as ordered by
 * osenv_device_lookup) of the appropriate interface.  Returns non-zero
 * if no matching TMCP-confgiured interface was found.
 */
int tmcp_addr2if(struct in_addr *addr, int *oskitifn);

/*
 * Using TMCP provided info, map the provided OSKit interface number
 * (as ordered by osenv_device_lookup) to an IP address and mask for
 * that interface.  If the routine returns zero, *addr and *mask contain
 * the TMCP configured IP address/mask of that interface.  Returns non-zero
 * if that interface is not in the TMCP-provided info.
 */
int tmcp_if2addr(int oskitifn, struct in_addr *addr, struct in_addr *mask);

/*
 * Map the provided OSKit interface number (as ordered by osenv_device_lookup)
 * to the MAC address for the interface.  If the routine returns zero, *mac
 * contains the address of the interface.  Returns non-zero if that interface
 * does not exist.  Note that this doesn't require TMCP info right now,
 * but could someday if we every had a way to virtualize MAC addresses.
 */
int tmcp_if2mac(int oskitifn, unsigned char mac[6]);

/*
 * Log a message in the TMCD log buffer.  This buffer is flushed to TMCD
 * by tmcp_shutdown.  The buffer is very small (1k) and intended only for
 * terse results and error messages.
 */
void tmcp_log(const char *fmt, ...);
void tmcp_vlog(const char *fmt, va_list args);

/*
 * XXX complete and total hack.
 * Allows an application to register a "control net" netio to send shutdown
 * messages on in the event that the shutdown takes place before the app
 * closes the interface in question.
 */
struct oskit_netio;
void tmcp_setsendnetio(struct oskit_netio *nio);

/*
 * Inform TMCD that the node is shutting down.
 * The indicated status value is returned to TMCD to report to the
 * experiment creator.  If sendlog is non-zero, any pending log information
 * (written via tmcp_vlog) is sent back to TMCD.
 *
 * This should be the last tmcp_* function called (duh!)
 */
void tmcp_shutdown(int status, int sendlog);

/*
 * Dump all state obtained via TMCP.
 */
void tmcp_dumpinfo(void);

