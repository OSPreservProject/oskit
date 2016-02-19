/*
 * Copyright (c) 1995-1996, 1998, 1999 University of Utah and the Flux Group.
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

#include <oskit/x86/pc/rtc.h>		/* RTC_BASEHI */
#include <oskit/x86/pc/phys_lmm.h>	/* phys_lmm_add */
#include <oskit/x86/types.h>		/* oskit_addr_t */
#include <oskit/x86/multiboot.h>	/* multiboot structs */
#include <oskit/x86/pc/base_multiboot.h> /* boot_info */
#include <oskit/x86/base_cpu.h>		/* base_cpu_setup */
#include <oskit/x86/base_vm.h>		/* kvtophys */
#include <oskit/exec/exec.h>		/* exec_load */
#include <oskit/exec/a.out.h>		/* struct exec */
#include <oskit/c/malloc.h>		/* malloc_reserve */
#include <oskit/c/string.h>		/* memcpy */
#include <oskit/c/stdio.h>		/* printf */
#include <oskit/c/assert.h>		/* assert */
#include <oskit/lmm.h>			/* lmm_remove_free */
#include <oskit/clientos.h>

#include "boot.h"


#define CMDLINEBMOD "bootadapter_cmdline"

/* We set this in our multiboot startup hacks if -d given. */
char *enable_debug_arg;

static struct multiboot_info *new_boot_info;
static struct multiboot_module *boot_mods;

struct multiboot_header boot_kern_hdr;
void *boot_kern_image;
static  struct exec_info boot_kern_info;

extern struct lmod
{
	void *start;
	void *end;
	char *string;
} boot_modules[];

static oskit_addr_t      symaddr;
static oskit_size_t        symsize;
static oskit_addr_t      straddr;
static oskit_size_t        strsize;

/*
 * Build a command line string from our argv, oskit_bootargv, and environ.
 * We have to do this rather than rely on boot_info.cmdline
 * because that is modified by base_multiboot_init_cmdline and
 * argv has pointers into it.
 *
 * The command line is in a special format understood by the oskit
 * multiboot startup code (see base_multiboot_init_cmdline.c).
 * Basically, the format separates booting-options and environment
 * variable settings from normal args to pass to main().  The format is like
 *	progname [<booting-options and foo=bar> --] <args to main>
 * For example
 *	kernel DISPLAY=host:0 -d -- -rf foo bar
 */
static void init_cmdline(int argc, char **argv)
{
	extern char **environ;
	char **p;
	char *buf;
	oskit_size_t bufsize;
	int i;
	struct lmod *lm;

	/* Look for a bsd bootadapter command line bootmod.
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
	 * Figure out how big our args will be and allocate the buf.
	 */
	bufsize = 0;
	bufsize += strlen(argv[0]) + 1;
	for (i = 0; i < oskit_bootargc; i++)
		bufsize += strlen(oskit_bootargv[i]) + 1;
	if (enable_debug_arg)
		bufsize += strlen(enable_debug_arg) + 1;
	for (p = environ; *p != NULL; p++)
		bufsize += strlen(*p) + 1;
	bufsize += strlen("--") + 1;
	for (i = 1; i < argc; i++)
		bufsize += strlen(argv[i]) + 1;
	if (lm->start)
		bufsize += lm->end - lm->start;
	bufsize += 1;			/* for NUL */
	buf = mustcalloc(bufsize, 1);
	assert(buf);

	/*
	 * Copy the args in.
	 */
	strcpy(buf, argv[0]), strcat(buf, " ");

	for (i = 0; i < oskit_bootargc; i++)
		strcat(buf, oskit_bootargv[i]), strcat(buf, " ");

	if (enable_debug_arg)
		strcat(buf, enable_debug_arg), strcat(buf, " ");

	for (p = environ; *p != NULL; p++)
		strcat(buf, *p), strcat(buf, " ");

	/* Everything after this will be passed directly to main. */
	strcat(buf, "--"), strcat(buf, " ");

	for (i = 1; i < argc; i++)
		strcat(buf, argv[i]), strcat(buf, " ");

	if (lm->start) {
		/*
		 * Have a command line bmod.
		 */
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

	/* Insert the command line into the new_boot_info structure.  */
	new_boot_info->cmdline = (oskit_addr_t)kvtophys(buf);
	new_boot_info->flags |= MULTIBOOT_CMDLINE;
}

static
int kimg_read(void *handle, oskit_addr_t file_ofs, void *buf, oskit_size_t size, oskit_size_t *out_actual)
{
	/* XXX limit length */
	memcpy(buf, boot_modules[0].start + file_ofs, size);
	*out_actual = size;
	return 0;
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
	if (!(section_type & EXEC_SECTYPE_ALLOC))
		return 0;

	assert(mem_size > 0);
	assert(mem_addr >= boot_kern_hdr.load_addr);
	assert(mem_addr+file_size <= boot_kern_hdr.load_end_addr);
	assert(mem_addr+mem_size <= boot_kern_hdr.bss_end_addr);

	memcpy(boot_kern_image + mem_addr - boot_kern_hdr.load_addr,
		boot_modules[0].start + file_ofs, file_size);

	return 0;
}

/*
 * Callback for reading the symbol table.
 */
static
int kimg_read_exec_3(void *handle, oskit_addr_t file_ofs, oskit_size_t file_size,
		     oskit_addr_t mem_addr, oskit_size_t mem_size,
		     exec_sectype_t section_type)
{
	if (section_type & EXEC_SECTYPE_AOUT_SYMTAB) {
		symaddr = (oskit_addr_t)boot_modules[0].start + file_ofs;
		symsize = file_size;
	}
	else if (section_type & EXEC_SECTYPE_AOUT_STRTAB) {
		straddr = (oskit_addr_t)boot_modules[0].start + file_ofs;
		strsize = file_size;
	}

	return 0;
}

static void init_symtab(struct multiboot_header *h)
{
	int err;
	void *symtab;

	/*
	 * XXX we only deal with aoutly files.
	 */
	if (! (h->flags & MULTIBOOT_AOUT_KLUDGE))
		return;

	/*
	 * Figure out where the symtab is.
	 */
	if ((err = exec_load(kimg_read, kimg_read_exec_3, 0,
			     &boot_kern_info)) != 0)
		panic("cannot load kernel image 3: error code %d", err);

	/*
	 * Copy the symtab into non-conflicting memory.
	 * We assume the strtab is right after the symtab.
	 * In a.out the first word of the strtab is the size,
	 * for the symtab we add size info in our copy.
	 */
	symtab = mustmalloc(sizeof(oskit_size_t) + symsize + strsize);
	*((oskit_size_t *)symtab) = symsize;
	memcpy(symtab + sizeof(oskit_size_t), (void *)symaddr, symsize + strsize);

	/*
	 * Register the symtab in the new_boot_info.
	 */
	new_boot_info->flags |= MULTIBOOT_AOUT_SYMS;
	new_boot_info->syms.a.tabsize = symsize + sizeof(oskit_size_t);
	new_boot_info->syms.a.strsize = strsize;
	new_boot_info->syms.a.addr    = (oskit_addr_t)kvtophys(symtab);

	printf("symtab at %#x, %d bytes; strtab at %#x, %d bytes\n",
	       symaddr, symsize,
	       straddr, strsize);
}

int
main(int argc, char **argv)
{
	struct multiboot_header *h;
	int i, err;

	oskit_clientos_init();

	printf("MultiBoot->MultiBoot boot adaptor (compiled %s)\n", __DATE__);

	if (boot_modules[0].start == 0)
		panic("This boot image contains no boot modules!?!?");

	/* Scan for the multiboot_header.  */
	for (i = 0; ; i += 4)
	{
		if (i >= MULTIBOOT_SEARCH)
			panic("kernel image has no multiboot_header");
		h = (struct multiboot_header*)(boot_modules[0].start+i);
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
		boot_kern_image = (void*)h + h->load_addr - h->header_addr;
	}
	else
	{
		/*
		 * No a.out-kludge information available;
		 * attempt to interpret the exec header instead,
		 * using the simple interpreter in libexec.a.
		 */

		/* Perform the "load" in two passes.
		   In the first pass, find the number of sections the load image contains
		   and reserve the physical memory containing each section.
		   Also, initialize the boot_kern_hdr to reflect the extent of the image.
		   In the second pass, load the sections into a temporary area
		   that can be copied to the final location all at once by do_boot.S.  */

		boot_kern_hdr.load_addr = 0xffffffff;
		boot_kern_hdr.load_end_addr = 0;
		boot_kern_hdr.bss_end_addr = 0;

		if ((err = exec_load(kimg_read, kimg_read_exec_1, 0,
				     &boot_kern_info)) != 0)
			panic("cannot load kernel image 1: error code %d", err);
		boot_kern_hdr.entry = boot_kern_info.entry;

		/* It's OK to malloc this before reserving the memory the kernel will occupy,
		   because do_boot.S can deal with overlapping source and destination.  */
		assert(boot_kern_hdr.load_addr < boot_kern_hdr.load_end_addr);
		assert(boot_kern_hdr.load_end_addr < boot_kern_hdr.bss_end_addr);
		boot_kern_image = malloc(boot_kern_hdr.load_end_addr - boot_kern_hdr.load_addr);

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

	/*
	 * Build a multiboot_info struct to pass to the kernel,
	 * mostly by copying the relevant portions from the
	 * boot_info we were booted with.
	 * We don't include the bmods, symbol table, or section stuff
	 * because that won't apply to the booted kernel.
	 */
	new_boot_info = mustcalloc(sizeof(*new_boot_info), 1);
	memcpy(new_boot_info, &boot_info, sizeof(struct multiboot_info));
	new_boot_info->flags &= ~MULTIBOOT_MODS;
	new_boot_info->flags &= ~MULTIBOOT_AOUT_SYMS;
	new_boot_info->flags &= ~MULTIBOOT_ELF_SHDR;
	init_cmdline(argc, argv);
	init_symtab(h);

	/* Initialize the boot module entries in the new_boot_info.  */
	for (i = 1; boot_modules[i].start; i++);
	new_boot_info->mods_count = i-1;
	if (new_boot_info->mods_count > 0) {
		new_boot_info->flags |= MULTIBOOT_MODS;
		boot_mods = (struct multiboot_module*)mustcalloc(
			new_boot_info->mods_count * sizeof(*boot_mods), 1);
		new_boot_info->mods_addr = kvtophys(boot_mods);
	}
	for (i = 0; i < new_boot_info->mods_count; i++)
	{
		struct lmod *lm = &boot_modules[1+i];
		struct multiboot_module *bm = &boot_mods[i];

		assert(lm->end >= lm->start);

		/* Try to leave the boot module where it is and pass its address.  */
		bm->mod_start = kvtophys(lm->start);
		bm->mod_end = kvtophys(lm->end);

		/* However, if the current location of the boot module
		   overlaps with the final location of the kernel image,
		   we have to move the boot module somewhere else.  */
		if ((bm->mod_start < boot_kern_hdr.bss_end_addr)
		    && (bm->mod_end > boot_kern_hdr.load_addr))
		{
			oskit_size_t size = lm->end - lm->start;
			void *newaddr = mustmalloc(size);

			printf("moving boot module %d from %08x to %08x\n",
				i, kvtophys(lm->start), kvtophys(newaddr));
			memcpy(newaddr, lm->start, size);

			bm->mod_start = kvtophys(newaddr);
			bm->mod_end = bm->mod_start + size;
		}

		/* Also provide the string associated with the module.  */
#ifdef DEBUG
		printf("lm->string '%s'\n", lm->string);
#endif
		{
			char *newstring = mustmalloc(strlen(lm->string)+1);
			strcpy(newstring, lm->string);
			bm->string = kvtophys(newstring);
		}

		bm->reserved = 0;
	}

	boot_start(new_boot_info);

	/* NOTREACHED */
	return -1;
}
