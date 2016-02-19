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

#include <oskit/startup.h>
#include <oskit/dev/dev.h>
#include <oskit/io/blkio.h>
#include <oskit/io/absio.h>
#include <oskit/svm.h>

#ifdef PTHREADS
#include <oskit/com/wrapper.h>
#include <oskit/threads/pthread.h>

#define	start_svm		start_svm_pthreads
#define	start_svm_on_blkio	start_svm_on_blkio_pthreads
#endif

/*
 * Start the SVM system on a specific oskit_blkio instance. In the pthreads
 * case, the bio should NOT be wrapped; it will be taken care of here.
 */
oskit_error_t
start_svm_on_blkio(oskit_blkio_t *bio)
{
#ifdef  PTHREADS
	{
		oskit_error_t	err;
		oskit_blkio_t	*wrappedbio;
		
		/*
		 * Wrap the partition blkio.
		 */
		err = oskit_wrap_blkio(bio,
			       (void (*)(void *))osenv_process_lock, 
			       (void (*)(void *))osenv_process_unlock,
			       0, &wrappedbio);

		if (err)
			panic("oskit_wrap_blkio() failed: errno 0x%x\n", err);

		/*
		 * Don't need the partition anymore, the wrapper has a ref.
		 */
		oskit_blkio_release(bio);
		bio = wrappedbio;
	}
#endif
	/*
	 * Allow for the possibility that the SVM system was already
	 * initialized without a pager bio, and that this might be a
	 * subsequent call to add paging to the mix. 
	 */
	svm_pager_init((oskit_absio_t *) bio);

	/*
	 * Don't need the partition anymore, SVM has a ref.
	 */
	oskit_blkio_release(bio);

	return 0;
}

/*
 * Start the SVM system on a disk/partition specification.
 *
 * Device initialization should already have been done.
 */
oskit_error_t
start_svm(const char *disk, const char *partition)
{
	oskit_error_t	err;
	oskit_blkio_t	*bio;

	/*
	 * If paging is not requested ...
	 */
	if (!disk) {
		svm_init((oskit_absio_t *) 0);
		return 0;
	}

	/* Otherwise open the disk partition. */
	osenv_process_lock();
	err = start_disk(disk, partition, 0, &bio);
	osenv_process_unlock();
	if (err)
		return err;

	return start_svm_on_blkio(bio);
}
