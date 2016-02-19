/*
 * Copyright (c) 1995-1998 University of Utah and the Flux Group.
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

#include <oskit/x86/pc/rtc.h>
#include <oskit/x86/pc/phys_lmm.h>
#include <oskit/x86/pc/dos_io.h>
#include <oskit/x86/types.h>
#include <oskit/x86/multiboot.h>
#include <oskit/x86/base_cpu.h>
#include <oskit/x86/base_vm.h>
#include <oskit/exec/exec.h>
#include <oskit/exec/a.out.h>
#include <oskit/c/malloc.h>
#include <oskit/c/string.h>
#include <oskit/c/stdio.h>
#include <oskit/c/fcntl.h>
#include <oskit/c/assert.h>
#include <oskit/lmm.h>

#include "boot.h"

struct multiboot_info *boot_info;
static struct multiboot_module *boot_mods;

struct multiboot_header boot_kern_hdr;
void *boot_kern_image;

static dos_fd_t boot_kern_fd;
static unsigned boot_kern_ofs;
static struct exec_info boot_kern_info;

static
int kimg_read(void *handle, oskit_addr_t file_ofs, void *buf,
	      oskit_size_t size, oskit_size_t *out_actual)
{
	oskit_addr_t newpos;
	int rc;

	rc = dos_seek(boot_kern_fd, boot_kern_ofs + file_ofs, 0, &newpos);
	if (rc != 0)
		return rc;
	return dos_read(boot_kern_fd, buf, size, out_actual);
}

static
int kimg_read_exec_1(void *handle, oskit_addr_t file_ofs, oskit_size_t file_size,
		     oskit_addr_t mem_addr, oskit_size_t mem_size,
		     exec_sectype_t section_type)
{
	if (!(section_type & EXEC_SECTYPE_ALLOC))
		return 0;

	assert(mem_size > 0);
	if (mem_addr < boot_kern_hdr.load_addr)
		boot_kern_hdr.load_addr = mem_addr;
	if (mem_addr+file_size > boot_kern_hdr.load_end_addr)
		boot_kern_hdr.load_end_addr = mem_addr+file_size;
	if (mem_addr+mem_size > boot_kern_hdr.bss_end_addr)
		boot_kern_hdr.bss_end_addr = mem_addr+mem_size;

	return 0;
}

static
int kimg_read_exec_2(void *handle, oskit_addr_t file_ofs, oskit_size_t file_size,
		     oskit_addr_t mem_addr, oskit_size_t mem_size,
		     exec_sectype_t section_type)
{
	oskit_size_t actual;
	int rc;

	if (!(section_type & EXEC_SECTYPE_ALLOC))
		return 0;

	assert(mem_size > 0);
	assert(mem_addr >= boot_kern_hdr.load_addr);
	assert(mem_addr+file_size <= boot_kern_hdr.load_end_addr);
	assert(mem_addr+mem_size <= boot_kern_hdr.bss_end_addr);

	rc = kimg_read(handle, file_ofs,
		       boot_kern_image + mem_addr - boot_kern_hdr.load_addr,
		       file_size, &actual);
	if (rc != 0)
		return rc;
	if (actual != file_size)
		return EX_CORRUPT;

	return 0;
}

static void load_kernel(int argc, char **argv)
{
	static char search[MULTIBOOT_SEARCH+sizeof(struct multiboot_header)];
	struct multiboot_header *h;
	oskit_size_t actual;
	int i, err;

	/* Scan for the multiboot_header.  */
	kimg_read(0, 0, search, sizeof(search), &actual);
	for (i = 0; ; i += 4)
	{
		if (i >= MULTIBOOT_SEARCH)
			panic("kernel image has no multiboot_header");
		h = (struct multiboot_header*)(search + i);
		if (h->magic == MULTIBOOT_MAGIC
		    && !(h->magic + h->flags + h->checksum))
			break;
	}
	if (h->flags & MULTIBOOT_MUSTKNOW & ~MULTIBOOT_MEMORY_INFO)
		panic("unknown multiboot_header flag bits %08x",
			h->flags & MULTIBOOT_MUSTKNOW & ~MULTIBOOT_MEMORY_INFO);
	boot_kern_hdr = *h;

	if (h->flags & MULTIBOOT_AOUT_KLUDGE)
	{
		/*
		 * Allocate memory and load the kernel into it.
		 * We do this before reserving the memory
		 * for the final kernel location,
		 * because the code in boot_start.c
		 * to copy the kernel to its final location
		 * can handle overlapping sources and destinations,
		 * and this way we may not need as much memory during bootup.
		 */
		boot_kern_image = mustmalloc(h->load_end_addr - h->load_addr);
		kimg_read(0, i + h->load_addr - h->header_addr, boot_kern_image,
			  h->load_end_addr - h->load_addr, &actual);
		assert(actual == h->load_end_addr - h->load_addr);

	}
	else
	{
		/*
		 * No a.out-kludge information available;
		 * attempt to interpret the exec header instead,
		 * using the simple interpreter in libexec.a.
		 * Perform the "load" in two passes.
		 * In the first pass,
		 * find the number of sections the load image contains
		 * and reserve the physical memory containing each section.
		 * Also, initialize the boot_kern_hdr
		 * to reflect the extent of the image.
		 * In the second pass, load the sections into a temporary area
		 * that can be copied to the final location
		 * all at once by do_boot.S.
		 */

		boot_kern_hdr.load_addr = 0xffffffff;
		boot_kern_hdr.load_end_addr = 0;
		boot_kern_hdr.bss_end_addr = 0;

		if ((err = exec_load(kimg_read, kimg_read_exec_1, 0,
				     &boot_kern_info)) != 0)
			panic("cannot load kernel image 1: error code %d", err);
		boot_kern_hdr.entry = boot_kern_info.entry;

		/*
		 * Allocate memory to load the kernel into.
		 * It's OK to malloc this
		 * before reserving the memory the kernel will occupy,
		 * because do_boot.S can deal with
		 * overlapping source and destination.
		 */
		assert(boot_kern_hdr.load_addr < boot_kern_hdr.load_end_addr);
		assert(boot_kern_hdr.load_end_addr < boot_kern_hdr.bss_end_addr);
		boot_kern_image = mustmalloc(boot_kern_hdr.load_end_addr -
					     boot_kern_hdr.load_addr);

		if ((err = exec_load(kimg_read, kimg_read_exec_2, 0,
				     &boot_kern_info)) != 0)
			panic("cannot load kernel image 2: error code %d", err);
		assert(boot_kern_hdr.entry == boot_kern_info.entry);
	}

	/*
	 * Reserve the memory that the kernel will eventually occupy.
	 * All malloc calls after this are guaranteed
	 * to stay out of this region.
	 */
	lmm_remove_free(&malloc_lmm,
			(void *)phystokv(boot_kern_hdr.load_addr),
			phystokv(boot_kern_hdr.bss_end_addr)
			- phystokv(boot_kern_hdr.load_addr));

	printf("kernel at %08x-%08x text+data %d bss %d\n",
		boot_kern_hdr.load_addr, boot_kern_hdr.bss_end_addr,
		boot_kern_hdr.load_end_addr - boot_kern_hdr.load_addr,
		boot_kern_hdr.bss_end_addr - boot_kern_hdr.load_end_addr);
	assert(boot_kern_hdr.load_addr < boot_kern_hdr.load_end_addr);
	assert(boot_kern_hdr.load_end_addr < boot_kern_hdr.bss_end_addr);
	if (boot_kern_hdr.load_addr < 0x1000)
		panic("kernel wants to be loaded too low!");
#if 0
	if (boot_kern_hdr.bss_end_addr > phys_mem_max)
		panic("kernel wants to be loaded beyond available physical memory!");
#endif
	if ((boot_kern_hdr.load_addr < 0x100000)
	    && (boot_kern_hdr.bss_end_addr > 0xa0000))
		panic("kernel wants to be loaded on top of I/O space!");

	boot_info = (struct multiboot_info*)mustcalloc(sizeof(*boot_info), 1);

	/* Build a command line to pass to the kernel.  */
#if 0
	init_cmdline();
#endif

	/*
	 * Find the extended memory available to the kernel we're loading.
	 * Note that although we ourselves (the DOS boot loader)
	 * use only the memory that DOS and the BIOS say is available,
	 * we use the values in NVRAM to pass to the kernel we're loading.
	 * This is because we must be a good citizen in the DOS environment
	 * since we rely on DOS I/O services to load the kernel,
	 * but once the kernel is loaded, it can blow away DOS completely
	 * and use all available memory for its own purposes.
	 */
	boot_info->flags |= MULTIBOOT_MEMORY;
	boot_info->mem_lower = (rtcin(RTC_BASEHI) << 8) | rtcin(RTC_BASELO);
	boot_info->mem_upper = (rtcin(RTC_DEXTHI) << 8) | rtcin(RTC_DEXTLO);
	printf("Memory reported by NVRAM: %d lower %d upper\n",
		boot_info->mem_lower, boot_info->mem_upper);
}

/*
 * Check for a self-contained boot image attached to our executable,
 * and if one exists, boot it.
 */
static void boot_self_contained(int argc, char **argv)
{
	struct loadhdr
	{
		unsigned magic;
		unsigned tabsize;
	};
	struct loadmod
	{
		oskit_addr_t mod_start;
		oskit_addr_t mod_end;
		oskit_addr_t string_start;
		oskit_addr_t string_end;
	};

	dos_fd_t exe_fd;
	unsigned short exehdr[3];
	oskit_size_t size, actual, newpos;
	unsigned off;
	struct loadhdr loadhdr;
	struct loadmod *loadmods;
	int loadnmods;
	int i;

	/* Open our executable file.  */
	if (dos_open(argv[0], O_RDONLY, 0, &exe_fd))
		return;

	/*
	 * Read the first part of the EXE header,
	 * to find out how long the original EXE file is.
	 */
	if (dos_read(exe_fd, exehdr, sizeof(exehdr), &actual)
	    || (actual != sizeof(exehdr))
	    || (exehdr[0] != 0x5a4d))
		return;
	off = (unsigned)exehdr[2] * 512;
	if (exehdr[1] != 0)
		off -= 512 - exehdr[1];

	/* Read the boot image digest header, if there is one. */
	if (dos_seek(exe_fd, off, SEEK_SET, &newpos)
	    || (newpos != off)
	    || (dos_read(exe_fd, &loadhdr, sizeof(loadhdr), &actual))
	    || (actual != sizeof(loadhdr))
	    || (loadhdr.magic != 0xf00baabb)
	    || (loadhdr.tabsize == 0))
		return;

	/*
	 * OK, there's definitely a self-contained boot image here,
	 * so now we're committed to using it.
	 * From now on we don't return from this function.
	 */

	/* Read the boot modules array */
	loadnmods = loadhdr.tabsize / sizeof(struct loadmod);
	size = loadnmods * sizeof(struct loadmod);
	loadmods = mustmalloc(size);
	if (dos_read(exe_fd, loadmods, size, &actual)
	    || (actual != size))
		goto corrupt;

	/* Load the kernel image */
	boot_kern_fd = exe_fd;
	boot_kern_ofs = off + loadmods[0].mod_start;
	load_kernel(argc, argv);

	/* Initialize the boot module entries in the boot_info.  */
	boot_info->flags |= MULTIBOOT_MODS;
	boot_info->mods_count = loadnmods-1;
	boot_mods = (struct multiboot_module*)mustcalloc(
		boot_info->mods_count * sizeof(*boot_mods), 1);
	boot_info->mods_addr = kvtophys(boot_mods);
	for (i = 0; i < boot_info->mods_count; i++)
	{
		struct loadmod *lm = &loadmods[1+i];
		struct multiboot_module *bm = &boot_mods[i];
		void *modaddr;
		char *modstring;

		/* Load the boot module */
		assert(lm->mod_end > lm->mod_start);
		size = lm->mod_end - lm->mod_start;
		modaddr = mustmalloc(size);
		if (dos_seek(exe_fd, off + lm->mod_start, SEEK_SET, &newpos)
		    || (newpos != off + lm->mod_start)
		    || dos_read(exe_fd, modaddr, size, &actual)
		    || (actual != size))
			goto corrupt;
		bm->mod_start = kvtophys(modaddr);
		bm->mod_end = bm->mod_start + size;

		/* Also provide the string associated with the module.  */
		assert(lm->string_end > lm->string_start);
		size = lm->string_end - lm->string_start;
		modstring = mustmalloc(size);
		if (dos_seek(exe_fd, off + lm->string_start, SEEK_SET, &newpos)
		    || (newpos != off + lm->string_start)
		    || dos_read(exe_fd, modstring, size, &actual)
		    || (actual != size))
			goto corrupt;
		bm->string = kvtophys(modstring);

		bm->reserved = 0;
	}

	/* Just boot it! */
	boot_start(boot_info);

corrupt:
	panic("corrupt boot image");
}

void main(int argc, char **argv)
{
	/* Try to boot a self-contained boot image first */
	boot_self_contained(argc, argv);

	panic("dosboot currently only supports self-contained images");
}
