/*
 * Copyright (c) 1997-1998 University of Utah and the Flux Group.
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

#include <sys/param.h>
#include <sys/resourcevar.h>

#include "glue.h"

#if 0
/*
 * some background from sys/ucred.h and sys/proc.h
 */
struct ucred {
        u_short cr_ref;                 /* reference count */
        uid_t   cr_uid;                 /* effective user id */
        short   cr_ngroups;             /* number of groups */
        gid_t   cr_groups[NGROUPS];     /* groups */
};

struct  pcred {
        struct  ucred *pc_ucred;        /* Current credentials. */
        uid_t   p_ruid;                 /* Real user id. */
        uid_t   p_svuid;                /* Saved effective user id. */
        gid_t   p_rgid;                 /* Real group id. */
        gid_t   p_svgid;                /* Saved effective group id. */
        int     p_refcnt;               /* Number of references. */
};
#endif 

/* 
 * note that this sets ucred.cr_uid to zero, giving us root privileges 
 */
static struct ucred ucred;
struct pcred oskit_freebsd_pcred = {&ucred /*, 0, 0, 0, 0, 0 */};
struct pstats oskit_freebsd_pstats;


/* XXX We want to set this up some time. */
char *panicstr;
