/*
 * Copyright (c) 1996, 1998-2001 University of Utah and the Flux Group.
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
 * This is a special multiboot_main for netboot.
 *
 * The only difference between this and the one in libkern is
 * that we copy our text and data into a special clone area.
 * We have to do that here before any of the initialized data changes.
 */

#include <oskit/x86/multiboot.h>
#include <oskit/x86/base_vm.h>
#include <oskit/x86/base_cpu.h>
#include <oskit/x86/proc_reg.h>
#include <oskit/x86/pc/reset.h>
#include <oskit/x86/pc/base_console.h>
#include <oskit/x86/pc/base_multiboot.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "reboot.h"

struct multiboot_info boot_info = {0,};		/* MUST be in data segment */
extern oskit_addr_t return_address;
static struct reboot_info reboot_info = {0,};	/* MUST be in data segment */

oskit_addr_t reboot_stub_loc;

extern int main(int argc, char *argv[], char *envp[]);
extern char **environ;

#ifdef __ELF__
extern void __oskit_init(void);
extern void __oskit_fini(void);
#endif

#ifdef GPROF
extern int enable_gprof;
extern void base_gprof_init();
#endif /* GPROF */

/*
 * Save all the info we will need to regain control
 * after the booted kernel exits.
 *
 * This info will go at the top of memory and boot_info.mem_upper
 * will be adjusted accordingly.
 *
 * This info consists of our text+data segments and the command line.
 * The multiboot_info struct passed by the boot loader (boot_info)
 * has already been copied into our data segment.
 * The other pointers in the multiboot_info info struct,
 * such as boot mods, symbols, or a memory map are not allowed and
 * are ignored.
 *
 * Be careful of changes to the data segment in this function before
 * our text+data is copied.
 * For example, printf at this point calls getchar->direct_cons_putchar
 * which has first-time code.
 *
 * Returns a NULL string upon success,
 * otherwise returns a string specifying what went wrong.
 * If the string begins with "WARNING" then something non-fatal occurred,
 * otherwise a fatal error occurred.
 * This return-value silliness is because the console isn't inited yet.
 */

/*
 * Translate an address in the running kernel's address space to
 * reference memory in the clone's address space.
 */
#define translate(addr) \
  ((oskit_addr_t)(clone_loc + ((oskit_addr_t)(addr) - (oskit_addr_t)&_start)))

void
patch_clone(void* what, int size)
{
	extern char _start;
	oskit_addr_t clone_loc = reboot_info.src;
        memcpy((void*)translate(what),what,size);
}

static char *
clone_self(void)
{
	extern char _start, edata;
	char *end_of_mem = (char *)(0x100000 + 1024*boot_info.mem_upper);
	oskit_size_t clone_size, cmdline_size;
	char *clone_loc, *cmdline_loc1, *cmdline_loc2;
	char *p;
	int i;

	/*
	 * Figure out where the clone will go.
	 * But don't copy it just yet, we have to do some modifications
	 * to our data section below, namely boot_info and reboot_info.
	 */
	clone_size = (oskit_addr_t)&edata - (oskit_addr_t)&_start;
	end_of_mem -= clone_size;
	clone_loc = end_of_mem;

	/*
	 * Deal with the command line.
	 *
	 * First we need to move it in case it is where the clone
	 * will be, but more importantly we must move it so it is
	 * above the to-be-adjusted mem_upper and will thus remain valid
	 * after the booted kernel exits.
	 * The movement of the string is kind of tricky because the desired
	 * new location might overlap the current location
	 * (in fact, when netboot regains control it will exactly
	 * overlap).
	 * We use memmove because it handles overlapping regions correctly.
	 *
	 * Second we must make two copies: one for the booted kernel to
	 * munge as needed (see base_multiboot_init_cmdline),
	 * and one to be used by the clone when it
	 * is re-animated.
	 *
	 * By now it should be clear why we don't accept boot mods, etc.
	 */
	cmdline_size = strlen((char *)boot_info.cmdline) + 1;
	cmdline_size = (cmdline_size + 3) &~ 3;	/* round up to mult of word */

	end_of_mem -= cmdline_size;
	memmove(end_of_mem, (char *)boot_info.cmdline,
		cmdline_size);		/* ok to copy extra goo */
	cmdline_loc1 = end_of_mem;

	end_of_mem -= cmdline_size;
	strcpy(end_of_mem, cmdline_loc1);
	cmdline_loc2 = end_of_mem;

	/* This is for the clone's copy. */
	boot_info.cmdline = (oskit_addr_t)cmdline_loc1;

	/*
	 * Fill in reboot_info, which the reboot stub will use to
	 * do its business.
	 */
	reboot_info.src   = (oskit_addr_t)clone_loc;
	reboot_info.size  = clone_size;
	reboot_info.arg   = translate(&boot_info);
	for (p = (char *)&reboot, i = 0; i < 1024; p++, i++)
		if (*(oskit_addr_t *)p == REBOOT_INFO_MAGIC)
			break;
	if (i == 1024)
		return "couldn't find REBOOT_INFO_MAGIC in reboot stub";
	reboot_info.magicloc = translate(p);

	/*
	 * Copy clone in and adjust boot_info's mem_upper and cmdline.
	 * The order is important here because we want our re-animated
	 * clone to see as much memory as we have and be able to munge
	 * the command line as needed.
	 */
/**/	memcpy(clone_loc, &_start, clone_size);
	/* Round end_of_mem down to 1K bndry. */
	end_of_mem = (char *)((oskit_addr_t)end_of_mem &~ 1023);
	boot_info.mem_upper = ((oskit_addr_t)end_of_mem - 0x100000)/1024;
	boot_info.cmdline = (oskit_addr_t)cmdline_loc2;

	/*
	 * Now that the clone is up there in high memory,
	 * we modify the reboot stub to access the reboot_info struct.
	 * Also, now is a fine time to set reboot_stub_loc.
	 */
	*(oskit_u32_t *)reboot_info.magicloc = translate(&reboot_info);
	reboot_stub_loc = translate(&reboot);

#if 0
	/*
	 * Debug printing.
	 * Be careful to do printf's after the kernel is stashed --
	 * this avoids changing the initialized data.
	 * also note that the console isn't inited yet,
	 * so this dump will go to the screen.
	 */
	printf("clone_size: %d\n", clone_size);
	printf("clone_loc: 0x%08x\n", clone_loc);
	hexdumpb(clone_loc, clone_loc, 16);
	printf("command_line1: \"%s\", command_line2: \"%s\"\n",
	       (char *)cmdline_loc1, (char *)cmdline_loc2);
	printf("reboot_info (at 0x%08x):\n", translate(&reboot_info));
	printf("\tsrc: 0x%08x, size: %d\n",
	       reboot_info.src, reboot_info.size);
	printf("\targ: 0x%08x, magicloc: 0x%08x\n",
	       reboot_info.arg, reboot_info.magicloc);
	multiboot_info_dump((struct multiboot_info *)reboot_info.arg);
	printf("adjusted mem_upper: 0x%08x\n", boot_info.mem_upper);
	printf("reboot_stub_loc: 0x%08x\n", reboot_stub_loc);
	hexdumpb(&reboot, &reboot, 16);
	hexdumpb(reboot_stub_loc, reboot_stub_loc, 16);
#endif

	/*
	 * Ignore boot_modules, syms, and memory map.
	 */
#define IGNORE(flag,desc) \
	if (boot_info.flags & flag) \
		return "WARNING: ignoring boot-loader provided " #desc "\n";
	IGNORE(MULTIBOOT_MODS,boot modules);
	IGNORE(MULTIBOOT_AOUT_SYMS,a.out syms);
	IGNORE(MULTIBOOT_ELF_SHDR,ELF section header);
	IGNORE(MULTIBOOT_MEM_MAP,memory map);
#undef IGNORE

	return NULL;
}

void
multiboot_main(oskit_addr_t boot_info_pa)
{
	int argc = 0;
	char **argv = 0;
	int i;
	char *info_string;

	/* Copy the multiboot_info structure into our pre-reserved area.
	   This avoids one loose fragment of memory that has to be avoided.  */
	boot_info = *(struct multiboot_info*)phystokv(boot_info_pa);

	/*
	 * Now we need to save all the info we will need to regain control
	 * after the booted kernel exits.
	 * We must do this now before any more of our data segment changes.
	 * This info will go at the top of memory and boot_info.mem_upper
	 * will be adjusted accordingly.
	 */
	info_string = clone_self();

	/* Identify the CPU and get the processor tables set up.  */
	base_cpu_setup();

	/* Initialize the memory allocator and find all available memory.  */
	base_multiboot_init_mem();

	/* Parse the command line into nice POSIX-like args and environment.  */
	base_multiboot_init_cmdline(&argc, &argv);

	/* Look for a return address. */
	for (i = 0; i < oskit_bootargc; i++)
		if (strcmp(oskit_bootargv[i], "-retaddr") == 0
		    && i+1 < oskit_bootargc) {
			return_address = strtoul(oskit_bootargv[i+1], 0, 0);
			break;
		}

	/* Enable interrupts, since we may be using remote gdb. */
	sti();

#ifdef FORCESERIALCONSOLE
	{
		char *ourargv[64];
		int ourargc;

		ourargc = oskit_bootargc;
		if (ourargc > 64-3)
			ourargc = 64-3;

		ourargv[0] = "-h";
		ourargv[1] = "-f";
		ourargc += 2;

		for (i = 2; i < ourargc; i++)
			ourargv[i] = oskit_bootargv[i-2];
		ourargv[i] = 0;

		base_console_init(ourargc, ourargv);
	}
#else
	/* Initialize the console */
	base_console_init(oskit_bootargc, oskit_bootargv);
#endif

	/* We do this now so it shows up on the console. */
	if (info_string) {
		if (strncmp(info_string, "WARNING", strlen("WARNING")) == 0)
			printf(info_string);
		else
			panic(info_string);
	}

#ifdef GPROF
	if (enable_gprof)
		base_gprof_init();
#endif

#ifdef __ELF__
	/* Make sure deinit code gets called on exit. */
	atexit(__oskit_fini);

	/* Call init code. */
	__oskit_init();
#endif

	/* Invoke the main program. */
	exit(main(argc, argv, environ));
}
