/*
 * Copyright (c) 1996-2001 University of Utah and the Flux Group.
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

#include <oskit/x86/base_vm.h>		/* phystokv */
#include <oskit/x86/types.h>		/* oskit_addr_t */
#include <oskit/x86/pc/phys_lmm.h>	/* phys_mem_max */
#include <oskit/exec/exec.h>		/* exec_load */
#include <stdio.h>			/* printf */
#include <stdlib.h>			/* atoi */
#include <assert.h>			/* assert */
#include <malloc.h>			/* mustcalloc */

#include "debug.h"
#include "fileops.h"
#include "boot.h"
#include "misc.h"
#include "timer.h"			/* ticks */

/*
 * Callback to transfer data for the kernel image from its src.
 */
static int
kimg_read(void *handle_notused,
	  oskit_addr_t file_ofs, void *buf, oskit_size_t size,
	  oskit_size_t *out_actual)
{
	int n, bytes_read, err;
	int offset;
	char *bufp = buf;
	int chunk_size;
	oskit_size_t got;
#if 0
	printf("kimg_read: file_ofs %x, size %x\n", file_ofs, size);
#endif
	bytes_read = 0;
	offset = file_ofs;
	while (size > 0) {
		chunk_size = min(FILE_READ_SIZE, size);
		err = fileread(offset, bufp, chunk_size, &got);
		if (err != 0) {
			printf("Unable to read text\n");
			return 1;	/* XXX Should be an EX_ constant */
		}
		n = (int) got;
		
		if (n == 0)
			break;		/* hit eof */
		offset += n;
		bufp += n;
		size -= n;
		bytes_read += n;
	}

        *out_actual = bytes_read;
        return 0;
}

/*
 * Callback to load the kernel for real.
 * Copies data from the current file into ki->ki_imgaddr.
 */
static int
kimg_load(void *handle,
	  oskit_addr_t file_ofs, oskit_size_t file_size,
	  oskit_addr_t mem_addr, oskit_size_t mem_size,
	  exec_sectype_t section_type)
{
	struct kerninfo *ki = handle;
	oskit_size_t got;
	int err;

#if 0
	printf("** load: file_ofs %x, file_size %x\n", file_ofs, file_size);
	printf("-- load: mem_addr %x, mem_size %x\n", mem_addr, mem_size);
	printf("-- load: sectype = %b\n", section_type, EXEC_SECTYPE_FORMAT);
#endif

        if (!(section_type & EXEC_SECTYPE_ALLOC))
                return 0;

	if ((err = kimg_read(ki, file_ofs,
			     ki->ki_imgaddr + mem_addr - ki->ki_mbhdr.load_addr,
			     file_size, &got)) != 0)
		return err;
	if (got != file_size)
		return EX_CORRUPT;

	return 0;
}

/*
 * Callback for the exec library that just tallys up the sizes.
 */
static int
kimg_tally(void *handle,
	   oskit_addr_t file_ofs, oskit_size_t file_size,
	   oskit_addr_t mem_addr, oskit_size_t mem_size,
	   exec_sectype_t section_type)
{
	struct multiboot_header *h = handle;

#if 0
	printf("** tally: file_ofs %x, file_size %x\n", file_ofs, file_size);
	printf("-- tally: mem_addr %x, mem_size %x\n", mem_addr, mem_size);
	printf("-- tally: sectype = %b\n", section_type, EXEC_SECTYPE_FORMAT);
#endif

	if (!(section_type & EXEC_SECTYPE_ALLOC))
		return 0;

	assert(mem_size > 0);
	if (mem_addr < h->load_addr)
		h->load_addr = mem_addr;
	if (mem_addr+file_size > h->load_end_addr)
		h->load_end_addr = mem_addr+file_size;
	if (mem_addr+mem_size > h->bss_end_addr)
		h->bss_end_addr = mem_addr+mem_size;

	return 0;
}

/*
 * Look in the first MULTIBOOT_SEARCH bytes of the file for the
 * MultiBoot header and puts it in ki->ki_mbhdr;
 *
 * Returns 0 if found, 1 otherwise.
 */
static int
find_multiboot_header(struct kerninfo *ki)
{
	int err;
	oskit_addr_t offset;
	char buf[FILE_READ_SIZE];
	oskit_size_t got;
	int i;
	struct multiboot_header *h;

	for (offset = 0; offset < MULTIBOOT_SEARCH; offset += FILE_READ_SIZE) {
		err = kimg_read(ki, offset, buf, FILE_READ_SIZE, &got);
		if (err)
			return 1;
		for (i = 0; i < got - sizeof(*h); i += 4) {
			h = (struct multiboot_header *)&buf[i];
			if (h->magic == MULTIBOOT_MAGIC
			    && (h->magic + h->flags + h->checksum) == 0) {
				ki->ki_mbhdr = *h;
				return 0;
			}
		}
	}
	return 1;
}

/*
 * Fills in certain fields of the ki_mbhdr multiboot_header using
 * the "executable interpreter" library.
 *
 * Returns 0 on success, 1 otherwise;
 */
static int
fill_in_multiboot_header(struct kerninfo *ki)
{
	struct multiboot_header *h = &ki->ki_mbhdr;
	struct exec_info xinfo;
	int err;

	h->load_addr = 0xffffffff;
	h->load_end_addr = 0;
	h->bss_end_addr = 0;

	err = exec_load(kimg_read, kimg_tally, &ki->ki_mbhdr, &xinfo);
	if (err) {
		printf("Can't tally sections: %s.\n", exec_strerror(err));
		return 1;
	}
	h->entry = xinfo.entry;

	return 0;
}

/*
 * Get the kernel and put it somewhere (not its final position).
 * The kerninfo struct we get passed has the ki_ip, ki_dirname, and ki_filename
 * members filled in.
 * We fill in the ki_mbhdr and ki_imgaddr parts.
 *
 * Returns 0 on success, 1 otherwise.
 */
int
getkernel_net(struct kerninfo *ki /* IN/OUT */)
{
	int err;
	exec_info_t xinfo;
	unsigned long loadticks;

	/*
	 * Contact the server and prepare to transfer.
	 */
	if (fileopen(ki->ki_transport, ki->ki_ip,
		     ki->ki_dirname, ki->ki_filename)) {
		printf("fileopen failed.\n");
		return 1;
	}

	/*
	 * Look in the file for the MultiBoot header.
	 */
	if (find_multiboot_header(ki) != 0) {
		printf("Can't find MultiBoot header in image.\n");
		return 1;
	}

	/*
	 * Fill in the rest of the MultiBoot header, if needed.
	 * Stuff like load address, bss size, etc.
	 */
	if (! (ki->ki_mbhdr.flags & MULTIBOOT_AOUT_KLUDGE)
	    && fill_in_multiboot_header(ki) != 0) {
		printf("Can't fill in MultiBoot header.\n");
		return 1;
	}

	/*
	 * Do a bunch of checks on the kernel load ranges.
	 */
	dprintf("Kernel will be at %08x-%08x text+data %d bss %d\n",
		ki->ki_mbhdr.load_addr, ki->ki_mbhdr.bss_end_addr,
		ki->ki_mbhdr.load_end_addr - ki->ki_mbhdr.load_addr,
		ki->ki_mbhdr.bss_end_addr - ki->ki_mbhdr.load_end_addr);
	assert(ki->ki_mbhdr.load_addr < ki->ki_mbhdr.load_end_addr);
	assert(ki->ki_mbhdr.load_end_addr < ki->ki_mbhdr.bss_end_addr);
	if (ki->ki_mbhdr.load_addr < 0x1000) {
		printf("kernel wants to be loaded too low!");
		return 1;
	}
	if (ki->ki_mbhdr.bss_end_addr > phys_mem_max) {
		printf("kernel wants to be loaded beyond available physical memory!");
		return 1;
	}
	if ((ki->ki_mbhdr.load_addr < 0x100000)
	    && (ki->ki_mbhdr.bss_end_addr > 0xa0000)) {
		printf("kernel wants to be loaded on top of I/O space!");
		return 1;
	}

	/*
	 * Now, read the whole thing in somewhere.
	 * It will be copied into its final resting home later,
	 * right before we jump in.
	 */
	printf("%s: loading...", ki->ki_filename);
	fflush(stdout);
	loadticks = ticks;
	ki->ki_imgaddr = mustcalloc(kerninfo_imagesize(ki), 1);
	if ((err = exec_load(kimg_read, kimg_load, (void *)ki, &xinfo)) != 0) {
		printf("Can't load kernel image: %s.\n", exec_strerror(err));
		return 1;
	}
	loadticks = ticks - loadticks;
	printf("done (%lu.%02lu seconds)\n", loadticks/100, loadticks%100);

	return 0;			/* yay */
}
