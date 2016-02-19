/*
 * Copyright (c) 1997-1998 The University of Utah and the Flux Group.
 * 
 * This file is part of the OSKit SMP Support Library, which is free software,
 * also known as "open source;" you can redistribute it and/or modify it under
 * the terms of the GNU General Public License (GPL), version 2, as published
 * by the Free Software Foundation (FSF).
 * 
 * The OSKit is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GPL for more details.  You should have
 * received a copy of the GPL along with the OSKit; see the file COPYING.  If
 * not, write to the FSF, 59 Temple Place #330, Boston, MA 02111-1307, USA.
 */

/*
 * Many of these should just be deleted from the source.
 */

/*
 * XXX: a.out build doesn't work properly if some symbols get renamed
 * via #defines (macro expansion problems in boot.S).
 */
#ifdef __ELF__
#define _SMP_TRAMP_32_ENTRY_ OSKIT_SMP__SMP_TRAMP_32_ENTRY_
#define _SMP_TRAMP_END_ OSKIT_SMP__SMP_TRAMP_END_
#define _SMP_TRAMP_START_ OSKIT_SMP__SMP_TRAMP_START_
#endif
#define active_kernel_processor OSKIT_SMP_active_kernel_processor
#define apic_addr OSKIT_SMP_apic_addr
#define apic_retval OSKIT_SMP_apic_retval
#define apic_version OSKIT_SMP_apic_version
#define boot_cpu_id OSKIT_SMP_boot_cpu_id
#define cpu_callin_map OSKIT_SMP_cpu_callin_map
#define cpu_data OSKIT_SMP_cpu_data
#define cpu_logical_map OSKIT_SMP_cpu_logical_map
#define cpu_number_map OSKIT_SMP_cpu_number_map
#define cpu_present_map OSKIT_SMP_cpu_present_map
#define io_apic_addr OSKIT_SMP_io_apic_addr
#define kernel_counter OSKIT_SMP_kernel_counter
#define kernel_flag OSKIT_SMP_kernel_flag
#define kernel_stacks OSKIT_SMP_kernel_stacks
#define num_processors OSKIT_SMP_num_processors
#define smp_boot_cpus OSKIT_SMP_smp_boot_cpus
#ifdef __ELF__
#define smp_boot_data OSKIT_SMP_smp_boot_data
#define smp_boot_func OSKIT_SMP_smp_boot_func
#define smp_boot_gdt_limit OSKIT_SMP_smp_boot_gdt_limit
#define smp_boot_gdt_linear_base OSKIT_SMP_smp_boot_gdt_linear_base
#define smp_boot_idt_limit OSKIT_SMP_smp_boot_idt_limit
#define smp_boot_idt_linear_base OSKIT_SMP_smp_boot_idt_linear_base
#define smp_boot_spin OSKIT_SMP_smp_boot_spin
#define smp_boot_stack OSKIT_SMP_smp_boot_stack
#define smp_callin OSKIT_SMP_smp_callin
#endif
#define smp_found_config OSKIT_SMP_smp_found_config
#define smp_num_cpus OSKIT_SMP_smp_num_cpus
#define smp_proc_in_lock OSKIT_SMP_smp_proc_in_lock
#define smp_process_available OSKIT_SMP_smp_process_available
#define smp_scan_config OSKIT_SMP_smp_scan_config
#define smp_threads_ready OSKIT_SMP_smp_threads_ready
#define smp_unmap_range OSKIT_SMP_smp_unmap_range
#define syscall_count OSKIT_SMP_syscall_count
