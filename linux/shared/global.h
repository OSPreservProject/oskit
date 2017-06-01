/*
 * Copyright (c) 1997, 1998, 1999, 2000 The University of Utah and the Flux Group.
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
 * Global symbol definitions to be included in all Linux fs or dev code.
 * These defines add OSKIT_LINUX_ prefixes to global Linux symbols
 * to ensure namespace cleanliness and prevent linking conflicts.
 * The Linux drivers themselves and the Linux glue code
 * should still continue to use the unprefixed names.
 */
/*
 * The symbols here are the ones from
 *
 *	$(OSKIT_SRCDIR)/linux/shared
 *	$(OSKIT_SRCDIR)/linux/shared/x86
 *	$(OSKIT_SRCDIR)/linux/shared/libc
 *	$(OSKIT_SRCDIR)/linux/src/lib
 *	$(OSKIT_SRCDIR)/linux/src/arch/i386/lib
 *	$(OSKIT_SRCDIR)/linux/src/arch/i386/kernel
 *
 * This was generated with the global.h.sh script.
 */
#ifndef _LINUX_SHARED_GLOBAL_H_
#define _LINUX_SHARED_GLOBAL_H_

#ifdef OSKIT_ARM32_SHARK
#define change_bit OSKIT_LINUX_change_bit
#define clear_bit OSKIT_LINUX_clear_bit
#define find_first_zero_bit OSKIT_LINUX_find_first_zero_bit
#define find_next_zero_bit OSKIT_LINUX_find_next_zero_bit
#define insl OSKIT_LINUX_insl
#define insw OSKIT_LINUX_insw
#define outsl OSKIT_LINUX_outsl
#define outsw OSKIT_LINUX_outsw
#define set_bit OSKIT_LINUX_set_bit
#define test_and_change_bit OSKIT_LINUX_test_and_change_bit
#define test_and_clear_bit OSKIT_LINUX_test_and_clear_bit
#define test_and_set_bit OSKIT_LINUX_test_and_set_bit
#define udelay OSKIT_LINUX_udelay
#endif

#define BIOS_START OSKIT_LINUX_BIOS_START
#define __down OSKIT_LINUX___down
#define __get_free_pages OSKIT_LINUX___get_free_pages
#define __up OSKIT_LINUX___up
#define __wake_up OSKIT_LINUX___wake_up
#define __wait_on_buffer OSKIT_LINUX___wait_on_buffer
#define _const_udelay OSKIT_LINUX__const_udelay
#define _ctype OSKIT_LINUX__ctype
#define _delay OSKIT_LINUX__delay
#define _down_failed OSKIT_LINUX__down_failed
#define _get_user_1 OSKIT_LINUX__get_user_1
#define _get_user_2 OSKIT_LINUX__get_user_2
#define _get_user_4 OSKIT_LINUX__get_user_4
#define _put_user_1 OSKIT_LINUX__put_user_1
#define _put_user_2 OSKIT_LINUX__put_user_2
#define _put_user_4 OSKIT_LINUX__put_user_4
#define _udelay OSKIT_LINUX__udelay
#define _up_wakeup OSKIT_LINUX__up_wakeup
#define _wake_up OSKIT_LINUX__wake_up
#define boot_cpu_data OSKIT_LINUX_boot_cpu_data
#define current OSKIT_LINUX_current
#define free_pages OSKIT_LINUX_free_pages
#define high_memory OSKIT_LINUX_high_memory
#define interruptible_sleep_on OSKIT_LINUX_interruptible_sleep_on
#define interruptible_sleep_on_timeout OSKIT_LINUX_interruptible_sleep_on_timeout
#define jiffies OSKIT_LINUX_jiffies
#define kdevname OSKIT_LINUX_kdevname
#define kfree OSKIT_LINUX_kfree
#define kfree_s OSKIT_LINUX_kfree_s
#define kmalloc OSKIT_LINUX_kmalloc
#define kmem_cache_alloc OSKIT_LINUX_kmem_cache_alloc
#define kmem_cache_create OSKIT_LINUX_kmem_cache_create
#define kmem_cache_free OSKIT_LINUX_kmem_cache_free
#define linux_cli OSKIT_LINUX_linux_cli
#define linux_oskit_osenv_device OSKIT_LINUX_linux_oskit_osenv_device
#define linux_oskit_osenv_driver OSKIT_LINUX_linux_oskit_osenv_driver
#define linux_oskit_osenv_intr OSKIT_LINUX_linux_oskit_osenv_intr
#define linux_oskit_osenv_ioport OSKIT_LINUX_linux_oskit_osenv_ioport
#define linux_oskit_osenv_irq OSKIT_LINUX_linux_oskit_osenv_irq
#define linux_oskit_osenv_isa OSKIT_LINUX_linux_oskit_osenv_isa
#define linux_oskit_osenv_log OSKIT_LINUX_linux_oskit_osenv_log
#define linux_oskit_osenv_mem OSKIT_LINUX_linux_oskit_osenv_mem
#define linux_oskit_osenv_pci_config OSKIT_LINUX_linux_oskit_osenv_pci_config
#define linux_oskit_osenv_sleep OSKIT_LINUX_linux_oskit_osenv_sleep
#define linux_oskit_osenv_timer OSKIT_LINUX_linux_oskit_osenv_timer
#define linux_restore_flags OSKIT_LINUX_linux_restore_flags
#define linux_save_flags OSKIT_LINUX_linux_save_flags
#define linux_save_flags_cli OSKIT_LINUX_linux_save_flags_cli
#define linux_sti OSKIT_LINUX_linux_sti
#define loops_per_sec OSKIT_LINUX_loops_per_sec
#define oskit_linux_init OSKIT_LINUX_oskit_linux_init
#define oskit_linux_mem_alloc OSKIT_LINUX_oskit_linux_mem_alloc
#define oskit_linux_mem_free OSKIT_LINUX_oskit_linux_mem_free
#define oskit_linux_osenv_init OSKIT_LINUX_oskit_linux_osenv_init
#define panic OSKIT_LINUX_panic
#define printk OSKIT_LINUX_printk
#define schedule OSKIT_LINUX_schedule
#define semaphore_wake_lock OSKIT_LINUX_semaphore_wake_lock
#define simple_strtoul OSKIT_LINUX_simple_strtoul
#define sleep_on OSKIT_LINUX_sleep_on
#define sprintf OSKIT_LINUX_sprintf
#define unlock_buffer OSKIT_LINUX_unlock_buffer
#define verify_area OSKIT_LINUX_verify_area
#define vmalloc OSKIT_LINUX_vmalloc
#define vfree OSKIT_LINUX_vfree
#define vsprintf OSKIT_LINUX_vsprintf

#endif /* _LINUX_SHARED_GLOBAL_H_ */
