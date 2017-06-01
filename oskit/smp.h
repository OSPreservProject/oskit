/*
 * Copyright (c) 1996, 1998 University of Utah and the Flux Group.
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

#ifndef _OSKIT_SMP_H_
#define _OSKIT_SMP_H_

#include <oskit/compiler.h>

#undef SMP_DEBUG

#include <oskit/machine/smp.h>

/*
 * Header file for SMP-specific functions
 */

/*
 * General Notes:
 * 1. Processors are not assumed to be numbered sequentially.
 * 2. smp_get_num_cpus() may take a LONG time (seconds?) the
 *    first time it is called.
 * XXX == only if called before smp_init!!
 *
 * 3. The division of labor between smg_get_num_cpus and
 *    smp_startup_processor is arbitrary, and transparent to
 *    the programmer (except for elapsed time), and may change.
 */


OSKIT_BEGIN_DECLS
/*
 * This gets everything going...
 * This should be the first SMP routine called.
 * It returns 0 on success (if it found a SMP system).
 * Success doesn't mean more than one processor, although
 * failure indicates only one.
 */
int smp_init(void);



/*
 * This routine returns the current processor number (UID)
 * It is only valid after smp_init() has bee called.
 */
int smp_find_cur_cpu(void);


/*
 * This routine determines the number of processors available.
 * It is called early enough so that any information it may need is
 * still intact.
 */
int smp_get_num_cpus(void);



/*
 * This is an iterator function to get all the CPU ID's
 * Call it the first time with 0; subsequently with (res+1)
 * Returns (<0) if no such processor
 */
int smp_find_cpu(int first);




/*
 * This function causes the processor numbered to begin
 * executing the function passed, with the stack passed, and
 * the data_arg on the stack (so it can bootstrap itself).
 * Note: on the x86, it will already be in protected mode
 *
 * If the processor doesn't exist, it will return E_SMP_NO_PROC
 */
void smp_start_cpu(int processor_id, void (*func)(void *data),
		void *data, void *stack_ptr);



/*
 * This is a hook provided by the host OS to allow the smp library
 * to request physical memory be mapped into its virtual address.
 * This is called by smp_init_paging().
 */
oskit_addr_t smp_map_range(oskit_addr_t start, oskit_size_t size);

OSKIT_END_DECLS

/*
 * This is called by the OS when it is ready to turn on paging.
 * This causes the smp library to make call-backs to the OS
 * to map the regions that are SMP-specific.
 * On the x86, this means the APICS.
 */
int smp_init_paging(void);


/*
 * This sends an interrupt to the specified CPU.
 * A non-blocking signal; the OS has to provide any necessary
 * synchronization.
 */
void smp_message_pass(int target_cpu_num);

/* Set the flag to 1 for each processor alloed to receive IPIs */
extern int smp_message_pass_enable[];


#define E_SMP_NO_CONFIG	-135
#define E_SMP_NO_PROC	-579

#endif /* _OSKIT_SMP_H_ */
