/*
 * Copyright (c) 1996, 1997, 1998, 2000 The University of Utah and the Flux Group.
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


/*
 * Forward declarations.
 */
extern struct wait_queue *wait_for_request;
void linux_fdev_init_ts(struct task_struct *ts);

extern void timer_bh(void);
extern void tqueue_bh(void);
extern void immediate_bh(void);

extern void startrtclock(void);
extern void linux_version_init(void);
extern void linux_kmem_init(void);
extern void pci_init(void);
extern void linux_net_emulation_init (void);
extern void linux_intr_init(void);
extern void linux_softintr_init(void);
extern void device_setup(void);
extern void linux_printk(char *, ...);
extern void linux_timer_intr(void);

/*
 * Amount of contiguous memory to allocate for initialization.
 */
#define CONTIG_ALLOC    (512 * 1024)

