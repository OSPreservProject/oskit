/*
 * Copyright (c) 1996-1999 University of Utah and the Flux Group.
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
 * Interrupt driven console implementation code for kernels on the x86
 * that can replace the default polling implementation.
 */

#include <oskit/x86/pc/direct_cons.h>
#include <oskit/x86/pc/intr_cons.h>
#include <oskit/c/stdio.h>
#include <oskit/config.h>
#include <oskit/x86/pio.h>
#include <oskit/x86/pc/keyboard.h>

#include <oskit/dev/dev.h>


extern int direct_cons_flags;


#define KBD_DELAY 	60
#define KBD_RETRIES	10

#define KBD_IRQ	1

#define KBD_BUFSIZE 	64


/* 
 * A simple queue to buffer keyboard input.
 *
 * Note that head is volatile, since it is updated by the interrupt
 * code, behind the C compiler's back.  
 */
typedef struct {
    volatile int head;
    int tail;
    unsigned char buf[KBD_BUFSIZE];
} queue;

static queue kbd_queue;


#define queue_init(q) 	((q)->head = (q)->tail = 0)
#define queue_next(i) 	(((i) + 1) % KBD_BUFSIZE)
#define queue_full(q) 	(queue_next((q)->head) == q->tail)
#define queue_empty(q)	((q)->head == (q)->tail)

static int
queue_add(queue *q, int c) 
{
	if (!queue_full(q)) {
		q->buf[q->head] = c;
		q->head = queue_next(q->head);

		return 1;
	}

	return 0;
}

static int
queue_remove(queue *q)
{
	int c;

	if (!queue_empty(q)) {
		c = q->buf[q->tail];
		q->tail = queue_next(q->tail);

		return c;
	}

	return -1;
}



static void
kbd_wait_while_busy(void)
{
    int i = 1000;

    while (i--) {
        if ((inb(K_STATUS) & K_IBUF_FUL) == 0)
            break;
        osenv_timer_spin(KBD_DELAY);
    }
}


static void
kbd_write_cmd_byte(int cb)
{
	kbd_wait_while_busy();
	outb(K_CMD, KC_CMD_WRITE);	
	kbd_wait_while_busy();
	outb(K_RDWR, cb);
}


static void 
kbd_intr(void *d)
{
	int c;
	
	c = direct_cons_trygetchar();

	if (c >= 0)
		queue_add(&kbd_queue, c);
}


int 
intr_cons_init(void)
{
	int c, i, retries = KBD_RETRIES;

	/* Write the command byte to make sure that IRQ generation is off. */
	kbd_write_cmd_byte((K_CB_SCAN|K_CB_INHBOVR|K_CB_SETSYSF));

	/* empty keyboard buffer. */
	while (inb(K_STATUS) & K_OBUF_FUL) {
		osenv_timer_spin(KBD_DELAY);
		(void) inb(K_RDWR);
	}

	/* reset keyboard */
	while (retries--) {
		kbd_wait_while_busy();
		outb(K_RDWR, KC_CMD_PULSE);		
		i = 10000;
		while (!(inb(K_STATUS) & K_OBUF_FUL) && (--i > 0)) 
			osenv_timer_spin(KBD_DELAY);
		if ((c = inb(K_RDWR)) == K_RET_ACK)
			break;
	}

	if (retries > 0) {	
		retries = KBD_RETRIES;

		while (retries--) {
			/* Need to wait a long time... */
			i = 10000;
			while (!(inb(K_STATUS) & K_OBUF_FUL) && (--i > 0)) 
				osenv_timer_spin(KBD_DELAY * 10);
			if ((c = inb(K_RDWR)) == K_RET_RESET_DONE)
				break;
		}
	}

	/* XXX should we do something more drastic? (like panic?) */
	if (retries == 0)
		printf("%s: keyboard won't reset!\n", __FUNCTION__);

	/* Now turn on IRQ generation. */
	kbd_write_cmd_byte((K_CB_SCAN|K_CB_INHBOVR|K_CB_SETSYSF|K_CB_ENBLIRQ));

	osenv_irq_alloc(KBD_IRQ, kbd_intr, 0, 0);

	return 1;
}

int
intr_cons_getchar(void) 
{
	while (!(direct_cons_flags & DC_NONBLOCK) && queue_empty(&kbd_queue))
		;
	       
	return queue_remove(&kbd_queue);
}


