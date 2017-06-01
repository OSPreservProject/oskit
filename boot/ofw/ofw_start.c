/*
 * Copyright (c) 1999, 2001 University of Utah and the Flux Group.
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

/*	$NetBSD: ofw.c,v 1.22 1999/03/29 10:02:19 mycroft Exp $	*/

/*
 * Copyright 1997
 * Digital Equipment Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and
 * copied only in accordance with the following terms and conditions.
 * Subject to these conditions, you may download, copy, install,
 * use, modify and distribute this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce
 *    and retain this copyright notice and list of conditions as
 *    they appear in the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Digital Equipment Corporation. Neither the "Digital Equipment
 *    Corporation" name nor any trademark or logo of Digital Equipment
 *    Corporation may be used to endorse or promote products derived
 *    from this software without the prior written permission of
 *    Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied
 *    warranties, including but not limited to, any implied warranties
 *    of merchantability, fitness for a particular purpose, or
 *    non-infringement are disclaimed. In no event shall DIGITAL be
 *    liable for any damages whatsoever, and in particular, DIGITAL
 *    shall not be liable for special, indirect, consequential, or
 *    incidental damages or damages for lost profits, loss of
 *    revenue or loss of use, whether such damages arise in contract,
 *    negligence, tort, under statute, in equity, at law or otherwise,
 *    even if advised of the possibility of such damage.
 */

/*
 * This is the OFW->Multiboot adaptor. Derived in large part from
 * routines in netbsd:/usr/src/sys/arch/arm32/ofw/ofw.c

 * Here is the plan. The OFW will not allocate any more memory once it
 * hands control to the client (kernel), unless we ask it to do so.
 * So, go find all the unused regions of physical memory. The adaptor
 * will map these in at a well-known location in virtual space, and
 * create a memory map table to pass in the boot info structure to the
 * kernel. The Image is linked (as per OFW spec) to be at 0xF0000000,
 * so we copy the kernel down to its load address in the new Oskit
 * space, which is defined to be 0x18008000. The boot modules are also
 * copied out.  The result is that all the space once occupied by the
 * boot image can be claimed by the multiboot initialization code in
 * the kernel.
 */

#include <oskit/types.h>
#include <oskit/machine/ofw/ofw.h>
#include <oskit/machine/ofw/ofw_console.h>
#include <oskit/machine/paging.h>
#include <oskit/page.h>
#include <oskit/machine/multiboot.h>
#include <oskit/machine/trap.h>
#include <oskit/machine/proc_reg.h>
#include <oskit/arm32/shark/phys_lmm.h>
#include <oskit/arm32/shark/base_vm.h>
#include <oskit/exec/exec.h>
#include <oskit/exec/a.out.h>
#include <oskit/c/stdlib.h>
#include <oskit/c/stdio.h>
#include <oskit/c/assert.h>
#include <oskit/c/alloca.h>
#include <oskit/c/string.h>
#include <oskit/clientos.h>

#if 0
#define DPRINTF(fmt, args... ) printf(__FUNCTION__  ": " fmt , ## args)
#else
#define DPRINTF(fmt, args... )
#endif
#define PANIC(fmt, args... )   panic("OFW->Multiboot: " fmt , ## args)

/* The multiboot header to be constructed and passed along */
static struct multiboot_info *boot_info;
static struct multiboot_module *boot_mods;
static struct multiboot_memmap *boot_map;
struct multiboot_header boot_kern_hdr;
void *boot_kern_image;

/* The mkofwimage sticks this in */
#define CMDLINEBMOD "bootadapter_cmdline"

/*
 * The mkofw process sticks this into the object file.
 */
extern struct lmod {
	void *start;
	void *end;
	char *string;
} boot_modules[];

/* Override to avoid linking in kernel stuff */
oskit_stream_t *console;	/* Currently selected console      */

/* Define common exit symbol to make posix lib happy. */
static void our_exit(int rc);
void (*oskit_libc_exit)(int) = our_exit;

/*
 * The command line args are stashed in the OFW. Suck them out!
 *
 * XXX Note that this is all wrong. The OFW command line args need to
 * parsed and passed to the main program of the boot adaptor, then
 * reparsed into command line args for the kernel being invoked. Later!
 *
 * The command line is in a special format understood by the oskit
 * multiboot startup code (see base_multiboot_init_cmdline.c).
 * Basically, the format separates booting-options and environment
 * variable settings from normal args to pass to main().  The format is like
 *	progname [<booting-options and foo=bar> --] <args to main>
 * For example
 *	kernel DISPLAY=host:0 -d -- -rf foo bar
 *
 * We allow a special command-line bootmod and place it after the "--".
 */
static char *
init_cmdline(void)
{
	int		chosen;
	int		len;
	char		*bootargsv, *buf;
	struct lmod	*lm;

	/* Look for a bootadapter command line bootmod.
	 *
	 * We wait and copy the contents of the command line bmod
	 * _after_ the standard command line, so it's contents can
	 * override the defaults.
	 */
	for (lm = &boot_modules[0]; lm->start; lm++) {
		if (!strcmp(CMDLINEBMOD, lm->string))
			break;
	}

	/*
	 * Ask the OFW how big the command line was.
	 */
	if ((chosen = OF_finddevice("/chosen")) == -1)
		PANIC("OF_finddevice(/chosen)");

	if ((len = OF_getproplen(chosen, "bootargs")) < 0)
		PANIC("OF_getproplen(bootargs)");

	if ((bootargsv = (char *) alloca(len)) == NULL)
		PANIC("alloca(%d)\n", len);

	/*
	 * Allocate space for the command line:
	 *  The space for the string passed up from the OFW,
	 *  plus the "-- " in case one is not in the OFW string.
	 *  plus the prog-name at the front, in this case, "kernel " (7 bytes),
	 *  plus the command line bmod, *if* there is one,
	 *  plus the null byte at the end.
         *
	 *  so, (len) + 3 + 7 + 1 + (optional bmod len)
	 */
	buf = mustcalloc((len + 11 + (lm->start ? lm->end - lm->start : 0) *
			  sizeof(char)), 1);

	/* We don't have a better choice, really. */
	strcpy(buf, "kernel ");

	/*
	 * Now suck it out of the OFW.
	 */
	if (len) {
		OF_getprop(chosen, "bootargs", bootargsv, len);
		strncat(buf, bootargsv, len);
		strcat(buf, " ");
	}

	/*
	 * Okay, just stick the bmod at the end. I'll worry about the rest
	 * of this later.
	 */

	/* Deal with a command line bmod, if we've got one. */
	if (lm->start) {
		strncat(buf, lm->start, lm->end - lm->start);

		/*
		 * Move all the entries after ours down one spot (to
		 * preserve bootmod ordering).
		 */
		while ((lm + 1)->start)
			*lm = *(++lm);

		/*
		 * Zero out the last entry's start address,
		 * since a zero starting address signifies the end of
		 * the boot_modules array.
		 */
		lm->start = 0;
	}

	/* Insert the command line into the boot_info structure.  */
	boot_info->cmdline = (oskit_addr_t)kvtophys(buf);
	boot_info->flags |= MULTIBOOT_CMDLINE;

	return buf;
}

static int
kimg_read(void *handle, oskit_addr_t file_ofs,
	  void *buf, oskit_size_t size, oskit_size_t *out_actual)
{
	/* XXX limit length */
	memcpy(buf, boot_modules[0].start + file_ofs, size);
	*out_actual = size;
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
	if (!(section_type & EXEC_SECTYPE_ALLOC))
		return 0;

	assert(mem_size > 0);
	assert(mem_addr >= boot_kern_hdr.load_addr);
	assert(mem_addr+file_size <= boot_kern_hdr.load_end_addr);
	assert(mem_addr+mem_size <= boot_kern_hdr.bss_end_addr);

#ifdef DEBUG
	printf("Copying kernel section from %p (%d bytes) to 0x%x\n",
	       boot_modules[0].start + file_ofs, file_size, mem_addr);
#endif

	memcpy((void *) mem_addr,
	       boot_modules[0].start + file_ofs, file_size);

	return 0;
}

/*
 * This will eventually be the entrypoint for the OFW adaptor. For now, its
 * just the kernel entrypoint, called out of multiboot.S.
 */
void
ofw_start(void *ofw_handle)
{
	int			phandle;
	int			i, len, availmem_count;
	struct ofw_memregion	*pavail, *pmem;
	struct multiboot_header *h;
	struct multiboot_memmap *pmap;
	void			(*entry)(void *, oskit_addr_t);
	char			*cmdline;
	oskit_addr_t		bootmod_offset;
	struct exec_info	boot_kern_info;
	oskit_error_t		err;
	
	OF_init(ofw_handle);
	ofw_base_console_init();

	printf("\nOFW->MultiBoot boot adaptor (compiled %s)\n", __DATE__);
#if 0	
	ofw_getvirtualtranslations();
#endif

	/*
	 * Temp hack until I get around to a proper base_trap_init(). This
	 * gives me an OSKit backtrace and register dump instead of that
	 * pointless "Data abort" message from the ROM.
	 */
	{
		unsigned int *vectors = (unsigned int *)0;
		extern int t_data_abort();
		
		vectors[T_DATA_ABORT + 16] = (unsigned int) t_data_abort;
	}

	/*
	 * Get the rest of the regions. We need to know how many slots
	 * to allocate for the mmap array, and we want some of that memory
	 * to initialize an lmm with so we can do some allocations.
	 */
	if ((phandle = OF_finddevice("/memory")) == -1)
		PANIC("OF_finddevice(/memory)");
	
	if ((len = OF_getproplen(phandle, "available")) <= 0)
		PANIC("OF_getproplen(available regions)");
	
	if ((pavail = (struct ofw_memregion *) alloca(len)) == NULL)
		PANIC("alloca(%d)", len);
	
	if (OF_getprop(phandle, "available", pavail, len) != len)
		PANIC("OF_getprop(available regions)");

	availmem_count = len / sizeof(struct ofw_memregion);

	/*
	 * Map the memory in at its kernel virtual address.
	 *
	 * XXX Skip first region. Breaks otherwise. See next comment.
	 */
	for (i = 0, pmem = &pavail[0]; i < availmem_count; i++, pmem++) {
		pmem->start = OF_decode_int((unsigned char *)&pmem->start);
		pmem->size = OF_decode_int((unsigned char *)&pmem->size);

		DPRINTF("avail region %d: 0x%x 0x%x\n", i,
			pmem->start, pmem->size);

		if (i < 1)
			continue;

		ofw_vmmap(pmem->start | KERNEL_VIRTUAL_REGION,
			  pmem->start, pmem->size, 0);
		memset((void *) (pmem->start | KERNEL_VIRTUAL_REGION),
		       0, pmem->size);
	}

	/*
	 * Give the LMM some memory to work with.
	 *
	 * XXX Skipping first two regions. Two reasons:
	 *
	 * 1) it breaks if I do. The memset above appears to wipe out the
	 * OFW's page tables. My guess is that the translation information
	 * returned from the OFW is incorrect, or more likely, my
	 * understanding of it is incorrect.
	 *
	 * 2) it leaves some memory for the OFW if it needs it later. This
	 * means we can more or less freely use the OFW's services, even in
	 * the kernel, at least until VM is turned on.
	 *
	 * Note that the LMM gets just a little bit of memory. It certianly
	 * does not get the lower portions of memory, since that is where
	 * the kernel is going, and I do not want to worry about copying
	 * the kernel around yet. Of course, the multiboot adaptor *will*
	 * have to worry about it. Netboot too.
	 */
	pmem = &pavail[2];
	phys_lmm_init();
	phys_lmm_add(pmem->start, pmem->size);

	/*
	 * Gotta do this so malloc works!
	 */
	oskit_clientos_init();

	/*
	 * Okay, now its time to do real work.
	 */
	if (boot_modules[0].start == 0)
		PANIC("This boot image contains no boot modules!?!?");

	/*
	 * Scan for the multiboot_header.
	 */
	for (i = 0; ; i += 4)
	{
		if (i >= MULTIBOOT_SEARCH)
			PANIC("kernel image has no multiboot_header");
		h = (struct multiboot_header*)(boot_modules[0].start+i);
		if (h->magic == MULTIBOOT_MAGIC
		    && !(h->magic + h->flags + h->checksum))
			break;
	}
	if (h->flags & MULTIBOOT_MUSTKNOW & ~MULTIBOOT_MEMORY_INFO)
		PANIC("unknown multiboot_header flag bits %08x",
		      h->flags & MULTIBOOT_MUSTKNOW & ~MULTIBOOT_MEMORY_INFO);
	
	boot_kern_hdr = *h;

	if (h->flags & MULTIBOOT_AOUT_KLUDGE) {
		boot_kern_image = (void*)h + h->load_addr - h->header_addr;
	}
	else {
		/*
		 * No a.out-kludge information available;
		 * attempt to interpret the exec header instead,
		 * using the simple interpreter in libexec.a.
		 */

		/*
		 * Perform the "load" in two passes.  In the first pass,
		 * figure out what the image looks like, and where it
		 * needs to land. In the second pass, load the image into
		 * its proper position. Remember, we assert that there
		 * will be no overlap. Deal with that another day.
		 */

		boot_kern_hdr.load_addr = 0xffffffff;
		boot_kern_hdr.load_end_addr = 0;
		boot_kern_hdr.bss_end_addr = 0;

		if ((err = exec_load(kimg_read, kimg_read_exec_1, 0,
				     &boot_kern_info)) != 0)
			PANIC("cannot parse kernel image: error code %d", err);

		boot_kern_hdr.entry = boot_kern_info.entry;
	}

#ifdef DEBUG
	printf("kernel at %08x-%08x text+data %d bss %d\n",
		boot_kern_hdr.load_addr, boot_kern_hdr.bss_end_addr,
		boot_kern_hdr.load_end_addr - boot_kern_hdr.load_addr,
		boot_kern_hdr.bss_end_addr - boot_kern_hdr.load_end_addr);
#endif
	assert(boot_kern_hdr.load_addr < boot_kern_hdr.load_end_addr);
	assert(boot_kern_hdr.load_end_addr < boot_kern_hdr.bss_end_addr);

	/*
	 * The kernel needs to load in the first 8megs of kv memory.
	 * It is actually only 7MB as we want to avoid some of the 4MB
	 * DMA-able memory area.  Since we only use DMA for sound cards,
	 * we only reserve ~1MB.
	 *
	 * Sorry, its a simple model to start with.
	 */
	if (boot_kern_hdr.load_addr < 0x18100000)
		PANIC("kernel wants to be loaded too low!");
	if (boot_kern_hdr.bss_end_addr > 0x18800000)
		PANIC("kernel does not fit in first SIMM!");
	
	/*
	 * In this implementation, we copy the kernel into its proper
	 * position. We can do this 'cause we know the kernel does not
	 * overlap with the adaptor, the modules, or anything else. Did I
	 * mention this is simple?
	 */
	if (h->flags & MULTIBOOT_AOUT_KLUDGE) {
#ifdef DEBUG
		printf("Copying kernel from %p (%d bytes) to 0x%x\n",
		       boot_kern_image,
		       boot_kern_hdr.load_end_addr - boot_kern_hdr.load_addr,
		       boot_kern_hdr.load_addr);
#endif
		memcpy((void *) boot_kern_hdr.load_addr, boot_kern_image,
		       boot_kern_hdr.load_end_addr - boot_kern_hdr.load_addr);
	}
	else {
		if ((err = exec_load(kimg_read, kimg_read_exec_2, 0,
				     &boot_kern_info)) != 0)
			panic("cannot copy kernel image: error code %d", err);
	}

	/*
	 * Must zero BSS as per multiboot spec.
	 */
	memset((void *) boot_kern_hdr.load_end_addr, 0,
	       boot_kern_hdr.bss_end_addr - boot_kern_hdr.load_end_addr);

	boot_info = (struct multiboot_info*) mustcalloc(sizeof(*boot_info), 1);

	/*
	 * Build a command line to pass to the kernel.
	 */
	cmdline = init_cmdline();
#ifdef DEBUG
	printf("cmdline: %s\n", cmdline);
#endif

	/*
	 * Build the memory map to pass along.
	 */
	boot_map = (struct multiboot_memmap *)
		mustcalloc(sizeof(struct multiboot_memmap),
			   availmem_count - 1);

	for (i = 1, pmem = &pavail[1], pmap = boot_map;
	     i < availmem_count; i++, pmem++, pmap++) {
		pmap->mem_base = (oskit_addr_t) pmem->start;
		pmap->mem_size = (oskit_size_t) pmem->size;
	}
	boot_info->flags |= MULTIBOOT_MEM_MAP;
	boot_info->mmap_count = availmem_count - 1;
	boot_info->mmap_addr  = (oskit_addr_t) kvtophys(boot_map);

	/*
	 * Initialize the boot module entries in the boot_info.
	 */
	for (i = 1; boot_modules[i].start; i++);
	boot_info->mods_count = i-1;
	if (boot_info->mods_count > 0) {
		boot_info->flags |= MULTIBOOT_MODS;
		boot_mods = (struct multiboot_module*)mustcalloc(
			boot_info->mods_count * sizeof(*boot_mods), 1);
		boot_info->mods_addr = kvtophys(boot_mods);
	}
	
	bootmod_offset = boot_kern_hdr.bss_end_addr;
	
	for (i = 0; i < boot_info->mods_count; i++) {
		struct lmod *lm = &boot_modules[1+i];
		struct multiboot_module *bm = &boot_mods[i];
		char *newstring;
		int bootmod_size = lm->end - lm->start;

		assert(lm->end >= lm->start);

		/*
		 * I think the easiest thing to do right now is move all
		 * the boot modules from the load address to an area just
		 * above the kernel. This has the added benefit that we
		 * can then release the entire area occupied by the adaptor
		 * plus kernel plus boot modules, which the kernel can then
		 * use for its lmm.
		 *
		 * XXX since we are moving them anyway, page align them
		 * while we are at it (in case we want to memory map
		 * them someday).
		 */
		bootmod_offset = round_page(bootmod_offset);
#ifdef DEBUG
		printf("OFW: Moving boot module: 0x%08x -> 0x%08x (%s)\n",
		       (unsigned int) lm->start, bootmod_offset, lm->string);
#endif

		memcpy((void *) bootmod_offset, lm->start, bootmod_size);

		bm->mod_start = kvtophys(bootmod_offset);
		bm->mod_end = kvtophys(bootmod_offset + bootmod_size);
		bootmod_offset += bootmod_size;

		/* Give the module entry a copy of the string. */
		newstring = mustmalloc(strlen(lm->string)+1);
		strcpy(newstring, lm->string);
		
		bm->string = kvtophys(newstring);
		bm->reserved = 0;
	}

	/*
	 * Invoke the kernel! Pass along the ofw_handle for now. Add to
	 * the boot_info structure later.
	 */
	printf("booting...\n\n");

	entry = (void *) boot_kern_hdr.entry;
	entry(ofw_handle, (oskit_addr_t) kvtophys(boot_info));

	/*
	 * Never returns.
	 */
	PANIC("YAK!");
}

static void
our_exit(int rc)
{
	OF_exit();
}
