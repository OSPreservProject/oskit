/*
 * Copyright (c) 2000, 2001 University of Utah and the Flux Group.
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
 * Boot a multiboot compliant kernel, such as netboot.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <oskit/x86/base_trap.h>
#include <oskit/x86/base_vm.h>
#include <oskit/x86/pc/base_i16.h>
#include <oskit/error.h>
#include <oskit/exec/exec.h>
#include <oskit/lmm.h>
#include <oskit/machine/phys_lmm.h>
#include <oskit/x86/pc/rtc.h>		/* RTC_BASEHI */

#include "boot.h"
#include "pxe.h"
#include "decls.h"

/*
 * There is no way to determine the filesize before we actually read it.
 * We need to know that so we can allocate the single large buffer that
 * the PXE wants. 
 * So, just pick a maximum size and hope we fit.
 */
#define BOOTSIZE	0x300000

/*
 * The entire image is downloaded in one shot. We stash it in a malloc
 * area and just skip through it as requested.
 */
static char		       *image;
static int			image_size;

/*
 * Various multiboot thingies used below. 
 */
static struct multiboot_info   *boot_info;
struct multiboot_header		boot_kern_hdr;
void			       *boot_kern_image;
static struct exec_info		boot_kern_info;

/*
 * Mostly taken from the linux boot program.
 */
static
int kimg_read(void *handle, oskit_addr_t file_ofs, void *buf,
		oskit_size_t size, oskit_size_t *out_actual)
{
	if (file_ofs + size > image_size)
		return OSKIT_E2BIG;
	
	*out_actual = size;
	memcpy(buf, &image[file_ofs], size);

	return 0;
}

static int
kimg_read_exec_1(void *handle, oskit_addr_t file_ofs, oskit_size_t file_size,
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

static int
kimg_read_exec_2(void *handle, oskit_addr_t file_ofs, oskit_size_t file_size,
		 oskit_addr_t mem_addr, oskit_size_t mem_size,
		 exec_sectype_t section_type)
{
	oskit_size_t actual;

	if (!(section_type & EXEC_SECTYPE_ALLOC))
		return 0;

	assert(mem_size > 0);
	assert(mem_addr >= boot_kern_hdr.load_addr);
	assert(mem_addr+file_size <= boot_kern_hdr.load_end_addr);
	assert(mem_addr+mem_size <= boot_kern_hdr.bss_end_addr);

	kimg_read(handle, file_ofs,
		  (void*)boot_kern_image + mem_addr - boot_kern_hdr.load_addr,
		  file_size, &actual);

	assert(actual == file_size);

	return 0;
}

/*
 * Load the kernel via our hacked up kimg reader code above.
 * Couple of points:
 * 1) Scan the image pointer directly for the multiboot header, since
 *    doing a kimg_read of the first 8K requires a place to put it, and
 *    just don't have the room to spare.
 * 2) Don't worry about a.out. Don't have the text space to spare.
 * 3) All this code comes from the linux boot adaptor, and then modified.
 */
static void
load_kernel(void)
{
	char *search;
	struct multiboot_header *h;
	int i, err;

	/* Scan for the multiboot_header.  */
	search = image;
	for (i = 0; ; i += 4)
	{
		if (i >= MULTIBOOT_SEARCH)
			PANIC("kernel image has no multiboot_header");
		h = (struct multiboot_header*)(search + i);
		if (h->magic == MULTIBOOT_MAGIC
		    && !(h->magic + h->flags + h->checksum))
			break;
	}

	if (h->flags & MULTIBOOT_MUSTKNOW & ~MULTIBOOT_MEMORY_INFO)
		PANIC("unknown multiboot_header flag bits %08x",
		      h->flags & MULTIBOOT_MUSTKNOW & ~MULTIBOOT_MEMORY_INFO);
	
	boot_kern_hdr = *h;

	if (h->flags & MULTIBOOT_AOUT_KLUDGE) {
		PANIC("MULTIBOOT_AOUT_KLUDGE");
	} else {
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
			PANIC("cannot load kernel image 1: error code %d",err);
		boot_kern_hdr.entry = boot_kern_info.entry;

		/*
		 * Allocate memory to load the kernel into.
		 * It's OK to malloc this
		 * before reserving the memory the kernel will occupy,
		 * because do_boot.S can deal with
		 * overlapping source and destination.
		 */
		assert(boot_kern_hdr.load_addr < boot_kern_hdr.load_end_addr);
		assert(boot_kern_hdr.load_end_addr<boot_kern_hdr.bss_end_addr);
		
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

	if ((boot_kern_hdr.load_addr < 0x100000)
	    && (boot_kern_hdr.bss_end_addr > 0xa0000))
		panic("kernel wants to be loaded on top of I/O space!");
}

static void
init_boot_info(char *filename, char *cmdline)
{
	void	*dest;

	/* Allocate memory for the boot_info structure and modules array.  */
	boot_info = (struct multiboot_info*)mustcalloc(sizeof(*boot_info), 1);
	boot_info->mods_count = 0;
	boot_info->mods_addr  = 0;

	/* Fill in the upper and lower memory size fields in the boot_info.  */
	boot_info->flags |= MULTIBOOT_MEMORY;

	/*
	 * Find lower and upper bounds of physical memory.
	 * We look in the NVRAM to find the size of base memory and
	 * use extended_mem_size() to find the size of extended memory.
	 */
	boot_info->mem_lower = (rtcin(RTC_BASEHI) << 8) | rtcin(RTC_BASELO);
	boot_info->mem_upper = extended_mem_size();

	/*
	 * A boot args,
	 */
	dest = mustcalloc(2048, 1);
	boot_info->cmdline = kvtophys(dest);
	boot_info->flags |= MULTIBOOT_CMDLINE;
	strcpy(dest, filename);
	strcat(dest, " ");

#ifdef FORCESERIALCONSOLE
	/* Force serial line console */
	strcat(dest, " -h -f ");
#endif
	if (!cmdline[0] || !strstr(cmdline, "--"))
		strcat(dest, " -- ");
	if (cmdline[0])
		strcat(dest, cmdline);

	printf("Kernel Command Line: %s\n", (char *) dest);

	/*
	 * These are command lines passed up from 16 bit mode. We don't
	 * deal with these.
	 */
	assert(*((unsigned short*)phystokv(0x90020)) != 0xA33F);

	/*
	 * No Boot module support. The loaded kernel needs to be wrapped
	 * in a multiboot adaptor if it wants boot modules. 
	 */
}

/*
 * Load netboot.
 */
int
load_multiboot_image(struct in_addr *tftpserver, char *filename, char *cmdline)
{
	t_PXENV_TFTP_READ_FILE		readfile;
	struct in_addr			theserver;
	struct multiboot_info	       *boot_info_copy;

	/*
	 * We need to build the multiboot header cause the piece of
	 * crap PXE trashes the strings. Actually, what gets trashed
	 * depends on what version/build of the PXE, but this appears
	 * to be the current problem.
	 *
	 * Call init_boot_info first. Since we cannot load just the
	 * file header, we cannot lmm_remove_free the area where the
	 * kernel is going, and the boot_info structure was getting
	 * trashed cause it landed (malloc) in the region that later
	 * got released with lmm_remove_free. So, we have to copy the
	 * boot info later. See below.
	 */
	DPRINTF("Creating Multiboot goo ...\n");
	init_boot_info(filename, cmdline);

	/*
	 * Where to get the file. Default to the DHCP server if none
	 * was specified.
	 */
	if (tftpserver && tftpserver->s_addr)
		theserver.s_addr = tftpserver->s_addr;
	else
		theserver.s_addr = serverip.s_addr;

	/*
	 * The FILE_SIZE TFTP command is an extension we don't support.
	 * So, just allocate a big chunk of memory and download the
	 * file. If it doesn't fit, well too bad.
	 */
	image = (char *) mustmalloc(BOOTSIZE);

	memset(&readfile, 0, sizeof(readfile));
	readfile.ServerIPAddress = *((IP4_t *) &theserver.s_addr);
	readfile.GatewayIPAddress = *((IP4_t *) &gatewayip.s_addr);
	readfile.BufferSize = BOOTSIZE;
	readfile.Buffer = (ADDR32_t) image;
	strcpy(readfile.FileName, filename);

	printf("Downloading: %s:%s\n", inet_ntoa(theserver), filename);

	pxe_invoke(PXENV_TFTP_READ_FILE, &readfile);
	if (readfile.Status) {
		printf(__FUNCTION__ ": Reading File. PXE error %d\n",
		       readfile.Status);
		if (readfile.Status == 1)
			printf("    Probably file doesn't exist or "
			       "is larger than %d bytes\n",
			       BOOTSIZE);
		return -1;
	}
	DPRINTF("... Got %d bytes\n", readfile.BufferSize);

	/*
	 * Set up so that we can "read" the file.
	 */
	image_size = readfile.BufferSize;

	load_kernel();
	DPRINTF("Cleaning up the PXE goo ...\n");
	pxe_cleanup();

	/*
	 * Create a copy of the boot_info structure. See comment above.
	 */
	boot_info_copy  = mustcalloc(sizeof(struct multiboot_info), 1);
	*boot_info_copy = *boot_info;
	if (boot_info->cmdline) {
		boot_info_copy->cmdline =
			kvtophys(strdup((char *)phystokv(boot_info->cmdline)));
	}
	
	DPRINTF("Booting the kernel ...\n");
	boot_start(boot_info_copy);

	/*
	 * Never returns, but of course
	 */
	
	return 0;
}
