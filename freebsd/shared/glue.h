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
#ifndef _FREEBSD_SHARED_GLUE_H_
#define _FREEBSD_SHARED_GLUE_H_

#include <sys/param.h>
#include <sys/proc.h>
#include "osenv.h"

extern struct pcred oskit_freebsd_pcred;
extern struct pstats oskit_freebsd_pstats;

#define OSKIT_FREEBSD_CREATE_CURPROC(proc)					\
	(proc).p_cred = &oskit_freebsd_pcred;				\
	(proc).p_stats = &oskit_freebsd_pstats;				\
	(proc).p_flag = P_INMEM;					\
	(proc).p_stat = SRUN;						\
	(proc).p_pid = 1;	/* all processes are process 1 */	\
	curproc = &(proc);

#ifdef DEBUG
#define OSKIT_FREEBSD_DESTROY_CURPROC(p)					\
	if (curproc != &(p))						\
		panic("fdev_freebsd: current process got munched");	\
	curproc = (struct proc*)0xdeadcccc;

#define OSKIT_FREEBSD_CHECK_CURPROC()						\
	if (curproc == (struct proc*)0xdeadcccc)			\
		panic("fdev_freebsd: no current process");
#else
#define OSKIT_FREEBSD_DESTROY_CURPROC(proc)
#define OSKIT_FREEBSD_CHECK_CURPROC()
#endif

/*
 * Make sure CPL is 0 while sleeping.  Since it is global, another
 * process-level thread could start with this CPL, instead of starting
 * at 0.
 *
 * Since curproc is global, it too must be properly saved and restored
 * across calls to osenv_sleep.
 */
#define OSKIT_FREEBSD_SLEEP(proc) ({					\
	unsigned __cpl;							\
	int      __flag;						\
									\
	osenv_sleep_init(&(proc)->p_sr);				\
	save_cpl(&__cpl);						\
	restore_cpl(0);							\
	__flag = osenv_sleep(&(proc)->p_sr);				\
	curproc = (proc);						\
	restore_cpl(__cpl);						\
	__flag; })

void oskit_freebsd_init(void);

#endif /* _FREEBSD_SHARED_GLUE_H_ */
