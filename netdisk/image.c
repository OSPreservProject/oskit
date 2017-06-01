/*
 * Copyright (c) 1995-2001 University of Utah and the Flux Group.
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
#undef NDEBUG

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <malloc.h>

#include <oskit/dev/dev.h>              /* All for diskwrite */
#include <oskit/dev/linux.h>
#include <oskit/io/blkio.h>
#include <oskit/fs/read.h>
#include <oskit/diskpart/diskpart.h>
#include <oskit/startup.h>
#ifdef  PTHREADS
#include <oskit/threads/pthread.h>
#include <oskit/dev/dev.h>		/* osenv_wrap_blkio */
#endif

#include "image.h"
#include "imagehdr.h"
#include "fileops.h"
#include "rpc.h"			/* XXX clear_read_cache() */
#include "perfmon.h"

/* Options */
#define DOINFLATE_DB	/* Double buffering decompression. See note below. */
#define SHOWPROGRESS	/* print something once in a while */
#undef	DOCHECKSUM	/* compute a checksum of the data read */
#undef  DONTWRITE	/* testing: define to test read speed of network */
#ifdef  FRISBEE
#undef  DOINFLATE_DB
#endif

/* Input buffer for filedata */
#define MAX_PARTS		30
#define DISKWRITE_SIZE		(32 * 1024)
#define DISKSECT_SIZE		512
static char			disk_buf[DISKWRITE_SIZE+DISKSECT_SIZE];
static int			minblock;

#include "zlib.h"
#define INFLATEBUF_SIZE	(32 * 1024)
static char		*inflate_buf;
#ifdef  DOINFLATE_DB
static char		*inflate_buf_swap;
static int		thiswrite = 0;
#endif
z_stream		d_stream; /* decompression stream */
static int		d_state;  /* Last return code */
oskit_size_t		ibsize    = 0;
oskit_size_t		ibleft    = 0;
oskit_size_t		lastwrite = 0;
int			morestuff = 0;
int			ranout, runahead, notahead, writingdisk;
int			decompress_nextchunk(int compressahead);
int			inflate_subblock(oskit_blkio_t *, oskit_off_t *);
static off_t		total = 0;
static int		writeimage_driver(struct in_addr ip,
					  char *dirname, char *filename,
					  oskit_blkio_t *part);

#ifdef DOCHECKSUM
unsigned		cksum(unsigned char *, int, unsigned);
static unsigned		datasum;
#endif

char			*prog = "NetDisk";

int
writeimage(struct in_addr ip, char *dirname, char *filename, char *diskspec)
{
	char		*diskname = 0;
	char		*partname = 0;
	unsigned long	err;
	oskit_blkio_t	*part = 0;
	int		retval = 1;  /* Set to 0 for success */

	/* Disknames typically start with a letter? */
	diskname = diskspec;
	if (diskname == 0 || diskname[0] == 0 || !isalpha(*diskname)) {
		printf("Bad disk specification: %s\n",
		       diskname ? diskname : "<null>");
		return 1;
	}

	/*
	 * The partition is optional but if it exists, disk/part are
	 * separated by a :.
	 */
	partname = diskname;
	strsep(&partname, ":");

	printf("%s: Disk `%s', Partition `%s'\n",
	       prog, diskname, partname ? partname : "");

	if (!inflate_buf)
	    inflate_buf = memalign(1024, INFLATEBUF_SIZE+DISKSECT_SIZE);
	if (!inflate_buf) {
	    printf("%s: Could not allocate inflate buffer\n", prog);
	    return 1;
	}
#ifdef	DOINFLATE_DB
	if (!inflate_buf_swap)
	    inflate_buf_swap = memalign(1024, INFLATEBUF_SIZE+DISKSECT_SIZE);
	if (!inflate_buf_swap) {
	    printf("%s: Could not allocate inflate buffer swap\n", prog);
	    return 1;
	}
#endif
#ifdef  PTHREADS
	osenv_process_lock();
#endif
	err = start_disk(diskname, partname, 0, &part);
#ifdef  PTHREADS
	osenv_process_unlock();
#endif
	if (err)
		return 1;
	
#ifdef  PTHREADS
	/* original blkio is released before returning */
	part = osenv_wrap_blkio(part);
#endif
	minblock = oskit_blkio_getblocksize(part);
	assert(minblock <= DISKSECT_SIZE);

	retval = writeimage_driver(ip, dirname, filename, part);

	oskit_blkio_release(part);
	return retval;
}

#ifndef FRISBEE
int
writeimage_driver(struct in_addr ip, char *dirname, char *filename,
		  oskit_blkio_t *part)
{
	oskit_size_t	got;
	oskit_off_t	inputoffset = 0;
	int		i, nblocks;
	struct blockhdr *blockhdr;
	unsigned long	err;
	int		retval = 1;  /* Set to 0 for success */
#ifdef  DOTIMEIT
	tstamp_t	stamp, estamp;
#endif
#ifdef  PTHREADS
	pthread_t	decompressor_pid = 0;
	void	       *decompressor_thread(void *arg);
	int		decompressor_status;
	pthread_attr_t	threadattr;
	struct sched_param	schedparam;
#endif
	if (fileinit(ip, dirname, filename)) {
		printf("%s: file_open failed.\n", prog);
		return 1;
	}

	/*
	 * Read the first subblock header to find out how many subblocks.
	 */
	err = fileread(0, disk_buf, DEFAULTREGIONSIZE, &got);
	if (err != 0) {
		printf("%s: error 0x%lx while reading header.\n", prog, err);
		return 1;
	}
	if (got < DEFAULTREGIONSIZE) {
		printf("%s: short (%d < %d) header read.\n", prog,
		       got, DEFAULTREGIONSIZE);
		return 1;
	}
	blockhdr = (struct blockhdr *) disk_buf;

	if (blockhdr->magic != COMPRESSED_MAGIC) {
		printf("Bad Magic Number!\n");
		return 1;
	}
	nblocks = blockhdr->blocktotal;

#if	defined(PTHREADS) && defined(DOINFLATE_DB)
	/*
	 * Thread to do decompressing during the disk waits. We start it
	 * a lower priority to make sure that it runs only when the main
	 * thread sleeps in the diskwrite!
	 */
	schedparam.priority = PRIORITY_NORMAL - 1;
	pthread_attr_init(&threadattr);
	pthread_attr_setschedparam(&threadattr, &schedparam);
	pthread_attr_setschedpolicy(&threadattr, SCHED_RR);
	
	pthread_create(&decompressor_pid,
		       &threadattr, decompressor_thread, (void *) 0);
#endif
	total = 0;
	GETTSTAMP(stamp);

	for (i = 0; i < nblocks; i++) {
		off_t		correction;
		
		/*
		 * Decompress one subblock at a time. After its done
		 * make sure the input file pointer is at a block
		 * boundry. Move it up if not. 
		 */
		if (inflate_subblock(part, &inputoffset)) {
			goto done2;
		}

		if (inputoffset & (SUBBLOCKSIZE - 1)) {
			correction = SUBBLOCKSIZE -
				(inputoffset & (SUBBLOCKSIZE - 1));

			inputoffset += correction;
		}
#ifdef SHOWPROGRESS
		if (i && ((i % 64) == 0)) {
			GETTSTAMP(estamp);
			SUBTSTAMP(estamp, stamp);
			printf("Wrote %qd bytes", total);
			PRINTTSTAMP(estamp);
#ifdef CYCLECOUNTS
			printf(", sleep cycles %qd", total_sleep_cycles);
			printf(", readwait cycles %qd", total_readwait_cycles);
#endif
			printf("\n");
		}
#endif
	}
	retval = 0;
	printf("\n%s: Success!\n", prog);

	GETTSTAMP(estamp);
	SUBTSTAMP(estamp, stamp);
	printf("Wrote %qd bytes ", total);
	PRINTTSTAMP(estamp);
#ifdef DOCHECKSUM
	printf(", checksum=0x%x\n", datasum);
#endif
	printf("\n");

 done2:
#if	defined(PTHREADS) && defined(DOINFLATE_DB)
	pthread_cancel(decompressor_pid);
	pthread_join(decompressor_pid, (void *) &decompressor_status);
#endif
	return retval;
}
#endif

#ifdef FRISBEE
int
writeimage_driver(struct in_addr ip, char *dirname, char *filename,
		  oskit_blkio_t *part)
{
	oskit_size_t	got;
	oskit_off_t	inputoffset = 0;
	int		i, nblocks;
	struct blockhdr *blockhdr;
	unsigned long	err;
	int		retval = 1;  /* Set to 0 for success */
#ifdef  DOTIMEIT
	tstamp_t	stamp, estamp;
#endif

	if (fileinit(ip, dirname, filename)) {
		printf("%s: file_open failed.\n", prog);
		return 1;
	}

	/*
	 * Read the first subblock header to find out how many subblocks.
	 */
	err = fileread(0, disk_buf, DEFAULTREGIONSIZE, &got);
	if (err != 0) {
		printf("%s: error 0x%lx while reading header.\n", prog, err);
		return 1;
	}
	if (got < DEFAULTREGIONSIZE) {
		printf("%s: short (%d < %d) header read.\n", prog,
		       got, DEFAULTREGIONSIZE);
		return 1;
	}
	blockhdr = (struct blockhdr *) disk_buf;

	if (blockhdr->magic != COMPRESSED_MAGIC) {
		printf("Bad Magic Number!\n");
		return 1;
	}
	nblocks = blockhdr->blocktotal;

#if	defined(PTHREADS) && defined(DOINFLATE_DB)
	/*
	 * Thread to do decompressing during the disk waits. We start it
	 * a lower priority to make sure that it runs only when the main
	 * thread sleeps in the diskwrite!
	 */
	schedparam.priority = PRIORITY_NORMAL - 1;
	pthread_attr_init(&threadattr);
	pthread_attr_setschedparam(&threadattr, &schedparam);
	pthread_attr_setschedpolicy(&threadattr, SCHED_RR);
	
	pthread_create(&decompressor_pid,
		       &threadattr, decompressor_thread, (void *) 0);
#endif

	GETTSTAMP(stamp);

	for (i = 0; i < nblocks; i++) {
		off_t		correction;
		
		/*
		 * Decompress one subblock at a time. After its done
		 * make sure the input file pointer is at a block
		 * boundry. Move it up if not. 
		 */
		if (inflate_subblock(part, &inputoffset)) {
			goto done2;
		}

		if (inputoffset & (SUBBLOCKSIZE - 1)) {
			correction = SUBBLOCKSIZE -
				(inputoffset & (SUBBLOCKSIZE - 1));

			inputoffset += correction;
		}
#ifdef SHOWPROGRESS
		if ((i % 64) == 0) {
			GETTSTAMP(estamp);
			SUBTSTAMP(estamp, stamp);
			printf("Wrote %qd bytes", total);
			PRINTTSTAMP(estamp);
#ifdef CYCLECOUNTS
			printf(", sleep cycles %qd", total_sleep_cycles);
			printf(", readwait cycles %qd", total_readwait_cycles);
#endif
			printf("\n");
		}
#endif
	}
	retval = 0;
	printf("\n%s: Success!\n", prog);

	GETTSTAMP(estamp);
	SUBTSTAMP(estamp, stamp);
	printf("Wrote %qd bytes ", total);
	PRINTTSTAMP(estamp);
#ifdef DOCHECKSUM
	printf(", checksum=0x%x\n", datasum);
#endif
	printf("\n");

 done2:
#if	defined(PTHREADS) && defined(DOINFLATE_DB)
	pthread_cancel(decompressor_pid);
	pthread_join(decompressor_pid, (void *) &decompressor_status);
#endif
	return retval;
}
#endif

int
inflate_subblock(oskit_blkio_t *blkio, oskit_off_t *inputoffset)
{
	unsigned long	err;
	oskit_size_t	got, buffd, cc, count;
	int		retval = 1;  /* Set to 0 for success */
	char		*bp;
	struct blockhdr *blockhdr;
	struct region	*curregion;
	off_t		offset, size;
	char		buf[DEFAULTREGIONSIZE];

	thiswrite = ibleft = lastwrite = morestuff = 0;

	d_stream.zalloc   = (alloc_func)0;
	d_stream.zfree    = (free_func)0;
	d_stream.opaque   = (voidpf)0;
	d_stream.next_in  = 0;
	d_stream.avail_in = 0;
	d_stream.next_out = 0;

	err = inflateInit(&d_stream);
	if (err != Z_OK) {
		printf("Error in inflateInit\n");
		return 1;
	}

	/*
	 * Read the subblock header.
	 */
	err = fileread(*inputoffset, buf, DEFAULTREGIONSIZE, &got);
	if (err != 0) {
		printf("%s: error 0x%lx while reading header.\n", prog, err);
		return 1;
	}
	if (got < DEFAULTREGIONSIZE) {
		printf("%s: short (%d < %d) header read.\n", prog,
		       got, DEFAULTREGIONSIZE);
		return 1;
	}
	blockhdr = (struct blockhdr *) buf;

	if (blockhdr->magic != COMPRESSED_MAGIC) {
		printf("Bad Magic Number!\n");
		return 1;
	}
	curregion  = (struct region *) (blockhdr + 1);

	/*
	 * Start reading the input file after the block header.
	 */
	*inputoffset += DEFAULTREGIONSIZE;
	
	/*
	 * Start with the first region. 
	 */
	offset = curregion->start * (off_t) DISKSECT_SIZE;
	size   = curregion->size  * (off_t) DISKSECT_SIZE;
	curregion++;
	blockhdr->regioncount--;

	/* Now we start reading full blocks at a time */
	clear_read_cache();

	if (0)
		printf("Decompressing: %12qd --> ", offset);

	while (1) {
		/*
		 * Read just up to the end of compressed data.
		 */
		if (blockhdr->size >= DISKWRITE_SIZE)
			count = DISKWRITE_SIZE;
		else
			count = blockhdr->size;

		err = fileread((oskit_addr_t) (*inputoffset),
			       disk_buf, count, &got);
		if (err) {
			printf("%s: NFS read of %d bytes "
			       "at offset %u FAILED, err=0x%lx\n",
			       prog, count, (unsigned)*inputoffset, err);
			retval = 1;
			goto done;
		}
		if (got != count) {
			if (got == 0) {
				printf("%s: NFS read of %d bytes "
				       "at offset %u FAILED, unexpected EOF\n",
				       prog, count, (unsigned)*inputoffset);
				break;
			}
			printf("%s: short NFS read (%d of %d bytes) "
			       "at offset %u, probably at EOF\n",
			       prog, got, count, (unsigned)*inputoffset);
		}
		blockhdr->size -= got;
		*inputoffset += got;
		buffd   = got;

		/*
		 * The inflate buffer is used to expand the deflated data.
		 *
		 * It may also hold up to ``minblock''-bytes of previously
		 * inflated but not written (residual) data.
		 *
		 * [inflate_buf - inflate_buf+ibleft] holds the residual data.
		 * On each pass we attempt to inflate up to INFLATEBUF_SIZE
		 * bytes starting at inflate_buf+ibleft.
		 *
		 * After the inflate() call, we write out as many minblock
		 * sized pieces of inflated data as we can, starting at
		 * inflate_buf.  The remainder of the data is moved to the
		 * beginning of the inflate buffer for the next pass.
		 */
		d_stream.next_in   = disk_buf;
		d_stream.avail_in  = buffd;
	inflate_again:
		if (decompress_nextchunk(0)) {
			retval = 1;
			goto done;
		}

	write_again:
		/*
		 * Ah, this is a new one that I had not considered.
		 * Its possible that that the input buffer has so little
		 * in it, that the output buffer will contain less than
		 * minblock bytes. This code arranges for the little piece
		 * to look like residual, and then we go around the loop
		 * again to get more input and do another decompress. If
		 * we did not do this, then this little piece would get
		 * lost because of the buffd calculation below. 
		 */
		if (ibsize < minblock) {
			ibleft    = ibsize;
			lastwrite = 0;
			morestuff = 0;
			assert(d_stream.avail_in == 0);
			continue;
		}
		
		/*
		 * We will write out as many minblock-sized
		 * pieces as possible.  After the write, we will move
		 * the residual up to the top of the inflate buffer
		 * for next time.
		 */
		bp = inflate_buf;
		lastwrite = buffd = ibsize & ~(minblock - 1);
		ibleft = ibsize - buffd;
		morestuff = 0;

#ifdef DOCHECKSUM
		datasum = cksum(bp, buffd, datasum);
#endif
		writingdisk = 1;
		while (buffd) {
			/*
			 * Write data only as far as the end of the current
			 * region. We know that regions are always aligned
			 * to a multiple of DISKSECT_SIZE.
			 */
			if (buffd < size)
				cc = buffd;
			else
				cc = size;
			thiswrite = cc;
#ifndef DONTWRITE
			err = oskit_blkio_write(blkio, bp, offset, cc, &got);
			if (err) {
				printf("%s: disk write of %d bytes "
				       "at offset %qu FAILED, err=0x%lx\n",
				       prog, cc, offset, err);
				retval = 1;
				goto done;
			}
			if (got != cc) {
				printf("%s: short disk write (%d of %d bytes) "
				       "at offset %qu\n",
				       prog, got, cc, offset);
				retval = 1;
				goto done;
			}
#endif
			buffd   -= cc;
			bp      += cc;
			size    -= cc;
			offset  += cc;
			total   += cc;

			/*
			 * Hit the end of the region. Need to figure out
			 * where the next one starts and bump the dstoff
			 * pointer up to it. We continue writing data at
			 * that new offset.
			 */
			if (! size) {
				off_t	newoffset;
				
				/*
				 * No more regions! Must be done.
				 */
				if (!blockhdr->regioncount)
					break;
				
				newoffset  = curregion->start *
					(off_t) DISKSECT_SIZE;
				size    = curregion->size  *
					(off_t) DISKSECT_SIZE;

				total += newoffset - offset;
				offset = newoffset;
				curregion++;
				blockhdr->regioncount--;
			}
		}
		writingdisk = 0;
		
		/*
		 * Account for any residual data.
		 * If there is some, we move it to the head of the
		 * inflate buffer.
		 */
		/*
		 * This will happen if the upcall decompresses more
		 * stuff while the last diskwrite was waiting to
		 * finish up.
		 */
		if (morestuff)
			goto write_again;
			
		/*
		 * This will happen if there is not enough room in the
		 * output buffer to decompress the entire input buffer.
		 */
		if (d_stream.avail_in)
			goto inflate_again;

		if (d_state == Z_STREAM_END)
			break;
	}
	if (blockhdr->regioncount) {
		printf("%s: Failed, %d more regions\n",
		       prog, blockhdr->regioncount);
		retval = 1;
		goto done;
	}
	if (size != 0 || blockhdr->size != 0) {
		printf("%s: Failed, sizes not zero: %qd,%lu\n",
		       prog, size, blockhdr->size);
		retval = 1;
		goto done;
	}

	if (0)
		printf("%10qd\n", total);

#ifdef DOCHECKSUM
	printf(", checksum=0x%x\n", datasum);
#endif
	retval = 0;
 done:
	return retval;
}

int
decompress_nextchunk(int compressahead)
{
#ifdef	DOINFLATE_DB
	char		*tmp;

	/*
	 * Double buffering decompression is required for decompressing
	 * during the disk write. Thats because FS compression makes it
	 * likely that it will take more than one disk write to consume
	 * a single buffer of uncompressed data. If we decompressed during
	 * disk writes from that buffer, than the original data would be
	 * overwritten before it went to disk! By using a double buffer,
	 * we can decompress the next chunk into the second buffer while
	 * waiting for the first to be completely drained to disk over
	 * multiple writes. Then we swap and do it again. Simple, eh?
	 */
	if (compressahead && !writingdisk)
		return 0;
#else
	if (compressahead)
		return 0;
#endif

	/*
	 * This ensures we only do one decompress from the upcall.
	 */
	if (morestuff)
		return 0;

	/*
	 * The decompress input buffer is empty, so nothing we can do.
	 */
	if (! d_stream.avail_in) {
		ranout++;
		return 0;
	}

	/*
	 * If the current write size is small, we don't gain anything by
	 * decompressing from the upcall. 
	 */
	if (compressahead && (thiswrite < (8 * 1024)))
		return 0;

	if (lastwrite) {
		assert(ibleft >= 0 && ibleft < DISKSECT_SIZE);
#ifdef	DOINFLATE_DB
		tmp = inflate_buf_swap;
		inflate_buf_swap = inflate_buf;
		inflate_buf = tmp;

		if (ibleft)
			memcpy(inflate_buf, inflate_buf_swap+lastwrite,ibleft);
#else
		if (ibleft)
			memcpy(inflate_buf, inflate_buf+lastwrite, ibleft);
#endif
	}
	else if (ibleft != ibsize)
		ibleft = 0;
	
	if (compressahead) {
		runahead++;
	}
	else {
		notahead++;
	}

	d_stream.next_out  = inflate_buf + ibleft;
	d_stream.avail_out = INFLATEBUF_SIZE;

	d_state = inflate(&d_stream, Z_SYNC_FLUSH);
	if (d_state != Z_OK && d_state != Z_STREAM_END) {
		printf("%s: inflate failed, err=%d\n", prog, d_state);
		return 1;
	}

	/*
	 * Total data in the inflate buffer is the amount we
	 * converted this time plus any residual we had last
	 * time.
	 */
	ibsize = (INFLATEBUF_SIZE - d_stream.avail_out) + ibleft;

	morestuff = 1;
	return 0;
}

#if	defined(PTHREADS) && defined(DOINFLATE_DB)
void *
decompressor_thread(void *arg)
{
	while (1) {
		pthread_testcancel();

		if (morestuff) {
			/*
			 * Only one other buffer. Once thats been filled,
			 * we yield to the main thread. I could use a
			 * mutex/cond pair (or a sleep/wakeup) here and
			 * sync with the main thread, but that would be a
			 * lot more overhead, and since there is nothing
			 * else to do anyway while waiting for the disk
			 * write to complete, might as well spin around in
			 * circles.
			 */
			sched_yield();
		}
		else {
			/*
			 * I don't want this decompression switched out
			 * before it is finished, which would happen if the
			 * disk write in progress completes before the
			 * decompression does. That would be bad!  I could
			 * create a new mutex and handle it by synchronzing
			 * with the main thread, but this is infinitely
			 * easier, since by locking the process lock, the
			 * main thread cannot return from the disk write
			 * until this is done.
			 */
			osenv_process_lock();
			decompress_nextchunk(1);
			osenv_process_unlock();
		}
	}
}
#endif

void
showdiskinfo(char *diskname)
{
	oskit_blkio_t *disk;
	unsigned dstart, dsize;
	int nparts;
	diskpart_t parray[MAX_PARTS], *dpart;
	char dstr[8];
	int err, i, j;
#ifdef  PTHREADS
	osenv_process_lock();
#endif  PTHREADS

	err = oskit_linux_block_open(diskname,
				     OSKIT_DEV_OPEN_READ|OSKIT_DEV_OPEN_WRITE,
				     &disk);
	if (err)
		goto bail;

	nparts = diskpart_blkio_get_partition(disk, parray, MAX_PARTS);
	if (nparts < 1) {
		off_t osize;

		dstart = 0;
		err = oskit_blkio_getsize(disk, &osize);
		if (err) {
			oskit_blkio_release(disk);
			goto bail;
		}
		dsize = (unsigned)(osize / DISKSECT_SIZE);
	} else {
		dstart = parray[0].start;
		dsize = parray[0].size;
	}
	snprintf(dstr, sizeof dstr, "%s", diskname);
	printf("Disk %s: sector range: [%u - %u]\n",
	       dstr, dstart, dstart + dsize-1);
	if (nparts < 1) {
		oskit_blkio_release(disk);
		goto bail;
	}

	/*
	 * Note: use BSD syntax
	 */
	dpart = parray[0].subs;
	for (i = 0; i < parray[0].nsubs; i++, dpart++) {
		if (dpart->size == 0)
			continue;
		snprintf(dstr, sizeof dstr, "s%d", i+1);
		printf("  Part %s: [%u - %u]\n",
		       dstr, dpart->start, dpart->start + dpart->size);
		for (j = 0; j < dpart->nsubs; j++) {
			if (dpart->subs[j].size == 0)
				continue;
			snprintf(dstr, sizeof dstr, "s%d%c",
				 i+1, 'a' + j);
			printf("    Part %s: [%u - %u]\n",
			       dstr, dpart->subs[j].start,
			       dpart->subs[j].start + dpart->subs[j].size);
		}
	}
	oskit_blkio_release(disk);
 bail:
#ifdef  PTHREADS
	osenv_process_unlock();
#endif  PTHREADS
}

void
showdisks(void)
{
	char	buf[32];
	int	i;

	for (i = 0; i < 4; i++) {
		sprintf(buf, "wd%d", i);

		showdiskinfo(buf);
	}
}
