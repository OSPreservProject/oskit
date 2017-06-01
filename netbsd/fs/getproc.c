/*
 * Copyright (c) 1997-1999 University of Utah and the Flux Group.
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

#include "getproc.h"
#include <oskit/c/assert.h>
#include <oskit/c/string.h>

static pid_t nextpid = 1;
extern struct vnode *rootvnode;

struct ourproc {
    struct proc _proc;	/* must be first */
    struct pcred _pcred;
    struct filedesc0 _fd;
    struct plimit _plimit;
    struct pstats _pstats;
    struct ourproc *_next;
};
struct ourproc *next_oproc;
static oskit_error_t newproc(struct proc **procp);

oskit_error_t getproc(struct proc **procp)
{
    oskit_principal_t *prin;
    oskit_identity_t id;
    oskit_error_t ferror;
    struct proc *proc;
    struct pcred *pcred;
    struct ucred *cred;
    struct plimit *limit;
    int index;

    if ((ferror = newproc(&proc)) != 0)
	    return ferror;

    pcred = proc->p_cred;
    pcred->p_refcnt = 1;
    cred  = proc->p_ucred;

    /*
     * Look to see if the cred is in use. Need a new one if so.
     */
    if (cred->cr_ref != 1) {
	    cred = crget();
	    if (!cred) {
		    ferror = OSKIT_ENOMEM;
		    goto bad;
	    }
	    crfree(proc->p_ucred);
	    proc->p_ucred = cred;
    }

    ferror = oskit_get_call_context(&oskit_principal_iid, (void *)&prin);
    if (!ferror) {
	ferror = oskit_principal_getid(prin, &id);
	if (ferror) {
		oskit_principal_release(prin);
		return ferror;
		goto bad;
	}

	if (id.ngroups > NGROUPS) {
		FREE(id.groups, M_TEMP);
		oskit_principal_release(prin);
		ferror = OSKIT_EINVAL;
		goto bad;
	}

	cred->cr_uid = id.uid;
	cred->cr_gid = id.gid;
	cred->cr_ngroups = id.ngroups;
	for (index = 0; index < id.ngroups; index++)
		cred->cr_groups[index] = id.groups[index];

	pcred->p_ruid = pcred->p_svuid = id.uid;
	pcred->p_rgid = pcred->p_svgid = id.gid;
	
	if (id.groups)
		FREE(id.groups, M_TEMP);

	oskit_principal_release(prin);
    }
    else {
	cred->cr_uid = cred->cr_gid = cred->cr_ngroups = 0;
	
	pcred->p_ruid = pcred->p_svuid = 0;
	pcred->p_rgid = pcred->p_svgid = 0;
    }

    proc->p_fd->fd_refcnt = 1;
    proc->p_fd->fd_cmask  = 0;	/* umask is applied by client.  */
    proc->p_fd->fd_rdir   = rootvnode = 0;

    limit = proc->p_limit;
    limit->p_refcnt = 1;
    limit->p_lflags = 0;
    for (index = 0; index < RLIM_NLIMITS; index++)
	limit->pl_rlimit[index].rlim_cur = 
		limit->pl_rlimit[index].rlim_max = RLIM_INFINITY;

    memset(proc->p_stats, 0, sizeof(struct pstats));
    
    if (nextpid == 0)
	nextpid = 1;
    proc->p_pid = nextpid++;    

    *procp = proc;
    curproc = proc;
    return 0;

 bad:
    prfree(proc);
    return ferror;
}

/*
 * Return a proc structure to the cache.
 */
void prfree(struct proc *proc)
{
    struct ourproc *oproc = (struct ourproc *) proc;

    oproc->_next = next_oproc;
    next_oproc   = oproc;
    curproc      = 0;
}

/*
 * Look for a cached proc structure. Otherwise, make a new one.
 */
static oskit_error_t
newproc(struct proc **procp)
{
    struct ourproc *oproc;
    struct proc *proc;
    struct ucred *cred;
    struct filedesc0 *fd;

    if ((oproc = next_oproc) != NULL) {
	next_oproc = oproc->_next;
	proc = &oproc->_proc;

	assert(proc->p_cred  == &oproc->_pcred);
	assert(proc->p_limit == &oproc->_plimit);
	assert(proc->p_stats == &oproc->_pstats);
	assert(proc->p_fd    == &oproc->_fd.fd_fd);
	
	*procp = proc;
	return 0;
    }

    oproc = malloc(sizeof *oproc, M_TEMP, M_WAITOK);
    if (!oproc)
	return OSKIT_ENOMEM;
    memset(oproc, 0, sizeof *oproc);
    proc = &oproc->_proc;

    cred = crget();
    if (!cred) {
	free(oproc, M_TEMP);
	return OSKIT_ENOMEM;
    }
    proc->p_cred  = &oproc->_pcred;
    proc->p_limit = &oproc->_plimit;
    proc->p_stats = &oproc->_pstats;    
    proc->p_ucred = cred;

    fd = &oproc->_fd;
    fd->fd_fd.fd_nfiles     = NDFILE;
    fd->fd_fd.fd_ofileflags = fd->fd_dfileflags;
    fd->fd_fd.fd_ofiles     = fd->fd_dfiles;
    proc->p_fd = &fd->fd_fd;

    *procp = proc;
    return 0;
}

