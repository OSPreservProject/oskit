/*
 * Copyright (c) 1996, 1998, 1999 University of Utah and the Flux Group.
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

#include "svm_internal.h"

#include <oskit/debug.h>
#include <oskit/c/malloc.h>
#include <oskit/io/blkio.h>
#include <oskit/dev/dev.h>
#include <oskit/dev/linux.h>
#include <oskit/diskpart/diskpart.h>
#include <oskit/machine/physmem.h>

/*
 * The ptov table stores physical to virtual translations (inverse map).
 * Used by the pageout code to implement a simple clock-sweep of the
 * physical pages to find candidates for pageout.
 */
oskit_addr_t	*svm_ptov;
int		svm_ptov_count;

/*
 * This table is kinda silly. Basically a free list of disk pages,
 * linked by table index.
 */
int		*svm_freepages;
int		svm_nextfreepage;

/*
 * Water marks for pageout. If the lmm has less than this amount
 * of free memory, pageout pages. Set as a percentage of real memory.
 */
oskit_size_t	svm_lowwater;
oskit_size_t	svm_highwater;

/*
 * ABSIO or BLKIO of the swap device.
 *
 * Disk partitions are blkio's, but do not export an absio interface,
 * but we still want to support absio interfaces. I know these
 * interfacs are identical, but is it wise to depend on that? For now,
 * lets assume they are different.
 */
oskit_absio_t   *svm_pagerabsio;
oskit_blkio_t   *svm_pagerblkio;

/*
 * Flag.
 */
int		svm_paging;

int
svm_pager_init(oskit_absio_t *absio)
{
	int		psize;
	int		dcount, dsize, i;
	oskit_off_t	disk_size;
	void		*handle;
	oskit_iunknown_t *iunknown = (oskit_iunknown_t *) absio;

	if (svm_paging)
		panic("svm_pager_init: "
		      "Already setup for paging to bio:%p\n", 
		      (svm_pagerabsio == ((oskit_absio_t *) 0)) ?
		      (void *) svm_pagerblkio : (void *) svm_pagerabsio);

	/*
	 * Make sure svm_init is done, but without the absio.
	 */
	if (!svm_enabled)
		svm_init((oskit_absio_t *) 0);

	/*
	 * Make sure we got an absio or a blkio. We add a reference.
	 */
	if (! oskit_iunknown_query(iunknown, &oskit_absio_iid, &handle))
		svm_pagerabsio = (oskit_absio_t *) handle;
	else if (! oskit_iunknown_query(iunknown, &oskit_blkio_iid, &handle))
		svm_pagerblkio = (oskit_blkio_t *) handle;
	else
		return OSKIT_EINVAL;

	/*
	 * Allocate the ptov table. We can use just plain old malloc.
	 */
	svm_ptov_count = (phys_mem_max - phys_mem_min) / PAGE_SIZE;
	psize = sizeof(oskit_addr_t) * svm_ptov_count;

	if ((svm_ptov = (oskit_addr_t *) smalloc(psize)) == NULL)
		panic("svm_pager_init: Could not smalloc ptov table");

#ifdef  DEBUG_SVM_PAGEOUT
	printf("svm_pager_init: "
	       "Allocated ptov: 0x%x %d bytes\n", (int) svm_ptov, psize);
#endif

	for (i = 0; i < svm_ptov_count; i++)
		svm_ptov[i] = SVM_NULL_PTOV;

	/*
	 * Make sure at least 20% of memory is available.
	 */
	svm_lowwater  = (svm_physmem_avail() / 10) * 2;
	svm_highwater = (svm_physmem_avail() / 10) * 3;

#ifdef  DEBUG_SVM_PAGEOUT
	printf("svm_pager_init: "
	       "Low water = %d bytes, High water = %d bytes\n",
	       svm_lowwater, svm_highwater);
#endif
	if (svm_pagerblkio)
		oskit_blkio_getsize(svm_pagerblkio, &disk_size);
	else
		oskit_absio_getsize(svm_pagerabsio, &disk_size);

#ifdef	DEBUG_SVM
	printf("svm_pager_init: disksize:%d\n", (int) disk_size);
#endif

	/*
	 * Allocate the freepage table. We can use just plain old malloc.
	 */
	dcount = (int) disk_size / PAGE_SIZE;
	dsize  = sizeof(int) * dcount;

	if ((svm_freepages = (int *) smalloc(dsize)) == NULL)
		panic("svm_pager_init: Could not smalloc freepage table");

	printf("svm_pager_init: "
	       "Allocated freepages: 0x%x %d bytes\n",
	       (int) svm_freepages, dsize);

	/*
	 * Build the freepage list.
	 */
	svm_nextfreepage = 0;
	for (i = 0; i < (dcount - 1); i++)
		svm_freepages[i] = i + 1;
	svm_freepages[dcount - 1] = -1;

	{
		oskit_u64_t	before, after;

		before = get_tsc();
		after  = get_tsc();

		printf("svm_pager_init: Timer: %d\n", (int) (after - before));
	}

	svm_paging = 1;

	return 0;
}

/*
 * Allocate a diskpage from the free list.
 */
int
svm_getdiskpage()
{
	int	diskpage;

	if ((diskpage = svm_nextfreepage) == -1)
		panic("svm_getfreepage: No free disk pages\n");

	svm_nextfreepage        = svm_freepages[diskpage];
	svm_freepages[diskpage] = -1;

	return diskpage;
}

/*
 * Deallocate a diskpage.
 */
void
svm_freediskpage(int diskpage)
{
	svm_freepages[diskpage] = svm_nextfreepage;
	svm_nextfreepage        = diskpage;
}

int		svm_swapread_time, svm_swapwrite_time;
int		svm_swapwrite_count, svm_swapread_count;

/*
 * Read and write pages to the swap partition.
 */
int
svm_swapwrite(int diskpage, oskit_addr_t pa)
{
	int		err, amt;
	oskit_off_t	offset = ptob(diskpage);
	oskit_size_t	size   = PAGE_SIZE;
	oskit_u32_t	before, after;

#ifdef DEBUG_SVM
	printf("svm_swapwrite: Writing %d bytes at offset %d 0x%x\n",
	       size, (int) offset, (int) pa);
#endif
	before = get_tsc();
	if (svm_pagerabsio)
		err = oskit_absio_write(svm_pagerabsio,
					(void *)pa, offset, size, &amt);
	else
		err = oskit_blkio_write(svm_pagerblkio,
					(void *)pa, offset, size, &amt);
	after  = get_tsc();
	if (after > before) {
		svm_swapwrite_time += (int) ((after - before) / 200);
		svm_swapwrite_count++;
	}
	if (err || amt != size) {
		printf("  Read WRITE %08x %d\n", err, amt);
		return(1);
	}
#ifdef DEBUG_SVM
	printf("Write done\n");
#endif
	return 0;
}

int
svm_swapread(int diskpage, oskit_addr_t pa)
{
	int		err, amt;
	oskit_off_t	offset = ptob(diskpage);
	oskit_size_t	size   = PAGE_SIZE;
	oskit_u32_t	before, after;

#ifdef DEBUG_SVM
	printf("Reading %d bytes at offset %d 0x%x\n",
	       size, (int) offset, (int) pa);
#endif
	before = get_tsc();
	if (svm_pagerabsio)
		err = oskit_absio_read(svm_pagerabsio,
				       (void *)pa, offset, size, &amt);
	else
		err = oskit_blkio_read(svm_pagerblkio,
				       (void *)pa, offset, size, &amt);
	after  = get_tsc();
	if (after > before) {
		svm_swapread_time += (int) ((after - before) / 200);
		svm_swapread_count++;
	}
	if (err || amt != size) {
		printf("  Read READ %08x %d\n", err, amt);
		return(1);
	}
#ifdef DEBUG_SVM
	printf("Read done\n");
#endif
	return 0;
}
