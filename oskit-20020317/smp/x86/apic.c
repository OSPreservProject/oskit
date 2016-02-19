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
 * This code is loosly based on the Linux message pass routines.
 * The purpose of this code is to provide an inter-processor interrupt.
 * The signal is asynchronous.  Higher-level OS code must handle
 * any associated data and required synchronization.
 */

#include <oskit/config.h>
#include <oskit/x86/spin_lock.h>
#include <oskit/smp.h>
#include <oskit/x86/smp.h>
#include <assert.h>
#include "asm-smp.h"
#include "i82489.h"

#include <stdio.h>

#if (SMP_IPI_VECTOR != 13)
#error Update APIC target irq
#endif

/*
 * This is set to 1 when it wants this to actually *DO* something
 */
int smp_message_pass_enable[NR_CPUS] = { 0, 0, 0, 0, };


/*
 * The SMP startup code requires .code16 support.
 * So don't bother compiling real versions of these functions
 * of we don't have it.
 */
#ifdef HAVE_CODE16

/*
 * smp_message_pass is the basic building block for all IPIs.
 * It is up the the OS programmer to prevent deadlock and to
 * provide synchronization.
 * Due to some brain-dammage, the APIC must be read between
 * every write to ensure corrext operation.
 */

void
smp_message_pass(int target)
{
	unsigned long cfg;
	int irq = 0x2d;		/* IRQ 13 */

	/* Ignore requests until the processor is ready to receive them. */
	if (!smp_message_pass_enable[target]) {
#ifdef DEBUG
		printf("message pass not enabled on cpu %d\n", target);
#endif
		return;
	}

#if 0
	printf("SMP message pass from %d to %d\n", smp_find_cur_cpu(),
		target);
#endif

	/* APIC should always be ready */
	cfg = apic_read(APIC_ICR);
	if (cfg & (1<<12))
		printf("APIC not ready\n");

	/*
	 * Program the APIC to deliver the IPI
	 */
	cfg = apic_read(APIC_ICR2);
	cfg &= 0x00FFFFFF;

	/* Target chip */
	apic_write(APIC_ICR2, cfg|SET_APIC_DEST_FIELD(target));
	cfg = apic_read(APIC_ICR);

	/* Clear bits */
	cfg &= ~0xFDFFF;

	/* Send an IRQ 13 */
	cfg |= APIC_DEST_FIELD|APIC_DEST_DM_FIXED|irq;

	/*
	 * Send the IPI. The write to APIC_ICR fires this off.
	 * This is an asynchronous notification.
	 */

	apic_write(APIC_ICR, cfg);
}


void
smp_apic_ack()
{
	apic_read(APIC_SPIV);   /* Dummy read */
	apic_write(APIC_EOI, 0);        /* ACK the APIC */
}

#else /* !HAVE_CODE16 */

void smp_message_pass(int target) {}
void smp_apic_ack() {}

#endif HAVE_CODE16

