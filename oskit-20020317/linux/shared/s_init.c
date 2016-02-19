/*
 * Copyright (c) 1996-1999 The University of Utah and the Flux Group.
 * 
 * This file is part of the OSKit Linux Glue Libraries, which are free
 * software, also known as "open source;" you can redistribute them and/or
 * modify them under the terms of the GNU General Public License (GPL),
 * version 2, as published by the Free Software Foundation (FSF).
 * 
 * The OSKit is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GPL for more details.  You should have
 * received a copy of the GPL along with the OSKit; see the file COPYING.  If
 * not, write to the FSF, 59 Temple Place #330, Boston, MA 02111-1307, USA.
 */
#include <linux/delay.h>
#include <linux/mm.h>
#include <linux/sched.h>

#include "shared.h"
#include "osenv.h"

#ifdef OSKIT_X86
#include <oskit/x86/base_cpu.h>
#endif

unsigned long volatile jiffies = 0;

#if 0
/*
 * Timing loop count.
 */
unsigned long loops_per_sec = 1;
#endif

#ifdef OSKIT_X86
/*
 * XXX: This gets referenced in a couple of places.
 */
struct cpuinfo_x86 boot_cpu_data;
#endif

/*
 * End of physical memory.
 */
void * high_memory;

/*
 * Pointer to a structure describing the current process-level activity.
 * It is the responsibility of the code using this library to make sure
 * this points to something meaningful when needed.
 * For example, see OSKIT_LINUX_CREATE_CURRENT.
 */
struct task_struct *current;


static void
bump_jiffies()
{
	jiffies++;
}

#if 0
/*
 * Calibrate delay loop.
 * Lifted straight from Linux.
 */
static void
calibrate_delay()
{
#ifdef LINUX_BOGOMIPS
	printk("Calibrating delay loop.. ");
	loops_per_sec = LINUX_BOGOMIPS * 500000;
	printk("pre-loaded - %lu.%02lu BogoMips\n",
	       loops_per_sec / 500000,
	       (loops_per_sec / 5000) % 100);
#else
	int ticks;

	printk("Calibrating delay loop.. ");
	while (loops_per_sec <<= 1) {
		/* Wait for "start of" clock tick.  */
		ticks = jiffies;
		while (ticks == jiffies)
			/* nothing */;
		/* Go .. */
		ticks = jiffies;
		__delay(loops_per_sec);
		ticks = jiffies - ticks;
		if (ticks >= HZ) {
			loops_per_sec = muldiv(loops_per_sec, HZ, ticks);
			printk("ok - %lu.%02lu BogoMips\n",
			       loops_per_sec / 500000,
			       (loops_per_sec / 5000) % 100);
			return;
		}
	}
	printk("failed\n");
#endif
}
#endif

void
oskit_linux_init()
{
	static int inited = 0;

	if (inited)
		return;
	inited = 1;

	high_memory = (void *) osenv_mem_phys_max();
	osenv_timer_register(bump_jiffies, HZ);

#if 0
	/* Set loop count. */
	calibrate_delay();
#endif

#ifdef OSKIT_X86
	boot_cpu_data.x86 = base_cpuid.family;
#if 0
	boot_cpu_data.loops_per_sec = loops_per_sec;
#endif
#endif
}
