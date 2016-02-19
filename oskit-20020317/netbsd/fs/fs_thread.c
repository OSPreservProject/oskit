/*
 * Copyright (c) 1997-2000 University of Utah and the Flux Group.
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
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
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
 */

/*
 * Implement the strategy routine as an async call that places the buf
 * pointer on a work queue. A thread continuously scans the work queue.
 */
#include "fs_glue.h"
#include <oskit/dev/dev.h>
#include <oskit/io/blkio.h>
#include <oskit/diskpart/diskpart.h>
#include <oskit/fs/netbsd.h>
#include <oskit/threads/pthread.h>

#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/proc.h>
#include <sys/malloc.h>

/*
 * This struct defines the list of worker threads. Each has a sleeprec
 * and a list pointer.
 */
struct sinfo {
	osenv_sleeprec_t	sleeprec;
	SIMPLEQ_ENTRY(sinfo)	sinfo_list;
};

int    fs_netbsd_thread_count		= 3;
static SIMPLEQ_HEAD(wd_queue, buf)	wd_queue;
static SIMPLEQ_HEAD(sr_queue, sinfo)	sr_queue;
extern void (*fs_netbsd_wdstrategy_routine)(struct buf *bp);

/*
 * The strategy routine simply adds a buf pointer to the list. No locking
 * is needed since only one thread can be operating inside here at a time
 * because of the process lock.
 */
void
fs_netbsd_threaded_wdstrategy(struct buf *bp)
{
	int		s;
	struct sinfo    *sinfop;

	s = splbio();
	SIMPLEQ_INSERT_TAIL(&wd_queue, bp, b_actlist);

	if ((sinfop = sr_queue.sqh_first) != NULL) {
		SIMPLEQ_REMOVE_HEAD(&sr_queue, sinfop, sinfo_list);
		osenv_wakeup(&sinfop->sleeprec, OSENV_SLEEP_WAKEUP);
	}
	splx(s);
}

/*
 * The actual strategy routine that is called from the wdloop.
 */
void
fs_netbsd_actual_wdstrategy(struct buf *bp)
{
	oskit_blkio_t	*bio = (oskit_blkio_t *)bp->b_dev;
	char		*datap;
	size_t		nbytes;
	oskit_u64_t	offset;
	int		s, rc;

	offset = bp->b_blkno * DEV_BSIZE;
	datap = (char *) bp->b_data;

	s = splbio();
	bp->b_resid = bp->b_bcount;
	do {
		if (bp->b_flags & B_READ) {
			SSTATE_DECL;

			SSTATE_SAVE;
			rc = oskit_blkio_read(bio, datap, offset,
					      bp->b_bcount, &nbytes);
			SSTATE_RESTORE;
		}
		else {
			SSTATE_DECL;

			SSTATE_SAVE;
			rc = oskit_blkio_write(bio, datap, offset,
					       bp->b_bcount, &nbytes);
			SSTATE_RESTORE;
		}
		if (rc != 0) {
			printf("wdstrategy:  oskit_blkio_%s() returned 0x%x\n",
			       (bp->b_flags & B_READ) ? "read" : "write", rc);
			break;
		}

		bp->b_resid -= nbytes;
		datap += nbytes;
		offset += nbytes;
	} while (bp->b_resid > 0);
	splx(s);

	biodone(bp);
	return;
}

/*
 * This is a thread that loops, looking for bufs to initiate I/O on.
 */
void *
fs_netbsd_threaded_wdloop(void *arg)
{
	struct sinfo    *sinfop = (struct sinfo *) arg;
	struct proc	*p;
	struct buf	*bp;
	int		s;
	sigset_t	sset;

	/*
	 * Do not let this thread be used to deliver signals.
	 */
	sset = -1;
	pthread_sigmask(SIG_BLOCK, &sset, 0);

	getproc(&p);
	osenv_process_lock();

	while (1) {
		s = splbio();
		osenv_sleep_init(&sinfop->sleeprec);
		if ((bp = wd_queue.sqh_first) != NULL) {
			SIMPLEQ_REMOVE_HEAD(&wd_queue, bp, b_actlist);
		}
		else {
			int	     saved_spl = reset_spl();

			SIMPLEQ_INSERT_TAIL(&sr_queue, sinfop, sinfo_list);
			osenv_sleep(&sinfop->sleeprec);
			restore_spl(saved_spl);
			curproc = p;
			continue;
		}
		splx(s);
		fs_netbsd_actual_wdstrategy(bp);
	}

	osenv_process_unlock();

	return 0;
}

int
fs_netbsd_threaded_init(oskit_osenv_t *osenv)
{
	int		rc, i;
	struct sinfo   *sinfop;
	pthread_t	tid;

	/*
	 * Increase buffers since multiple threads means more activity!
	 */
	if (nbuf == 0)
		nbuf = 128;		 /* XXX */

	rc = fs_netbsd_init(osenv);
	if (rc)
		return rc;

	fs_netbsd_wdstrategy_routine = fs_netbsd_threaded_wdstrategy;
	SIMPLEQ_INIT(&wd_queue);
	SIMPLEQ_INIT(&sr_queue);

	for (i = 0; i < fs_netbsd_thread_count; i++) {
		sinfop = malloc(sizeof *sinfop, M_TEMP, M_WAITOK);
		
		pthread_create(&tid, 0,
			       fs_netbsd_threaded_wdloop, (void *) sinfop);
		oskit_pthread_setprio(tid, PRIORITY_MIN);
	}
	
	return 0;
}
