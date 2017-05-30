/*
 * Copyright (c) 1997-1998, 2000 University of Utah and the Flux Group.
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
 * Copyright (c) 1983, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *      @(#)inet.h      8.1 (Berkeley) 6/2/93
 */

#ifndef _OSKIT_C_ARPA_INET_H_
#define _OSKIT_C_ARPA_INET_H_

/* External definitions for functions in inet(3) */

#include <oskit/machine/types.h>
#include <oskit/machine/endian.h>
#include <oskit/compiler.h>


/* Types used for Internet addresses and port numbers */
#ifndef _IN_ADDR_T_
#define _IN_ADDR_T_
typedef oskit_u32_t in_addr_t;
typedef oskit_u16_t in_port_t;
#endif /* !_IN_ADDR_T */

#ifndef _STRUCT_IN_ADDR_
#define _STRUCT_IN_ADDR_
/*
 * Internet address (a structure for historical reasons)
 */
struct in_addr {
	in_addr_t s_addr;
};
#endif /* !_STRUCT_IN_ADDR_ */


OSKIT_BEGIN_DECLS

/* Functions in UNIX 1994 standard */
/* `hton[ls]' and `ntoh[ls]' are now in <oskit/machine/endian.h>. */
in_addr_t	inet_addr(const char *);
in_addr_t	inet_lnaof(struct in_addr);
struct in_addr	inet_makeaddr(in_addr_t, in_addr_t);
in_addr_t	inet_netof(struct in_addr);
in_addr_t	inet_network(const char *);
char		*inet_ntoa(struct in_addr);

/* Nonstandard function(s) */
int              inet_aton (const char *, struct in_addr *);

OSKIT_END_DECLS

#endif /* !_OSKIT_C_ARPA_INET_H_ */
