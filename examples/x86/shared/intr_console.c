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
 *
 * This should be linked in before libkern, so that it overrides the base
 * console implementation found there.
 */

#include <oskit/x86/pc/com_cons.h>
#include <oskit/x86/pc/direct_cons.h>
#include <oskit/x86/pc/reset.h>
#include <oskit/x86/base_gdt.h>
#include <oskit/x86/debug_reg.h>
#include <oskit/c/stdio.h>
#include <oskit/c/unistd.h>
#include <oskit/c/stdlib.h>
#include <oskit/c/termios.h>
#include <oskit/tty.h>
#include <oskit/gdb.h>
#include <oskit/gdb_serial.h>
#include <oskit/base_critical.h>
#include <oskit/c/string.h>
#include <oskit/config.h>
#include <oskit/x86/pio.h>
#include <oskit/x86/proc_reg.h>
#include <oskit/x86/pc/keyboard.h>

#include <oskit/dev/dev.h>

static void our_exit(int rc);
/* A common symbol that is overridden */
void (*oskit_libc_exit)(int) = our_exit;

int cons_com_port = 1;			/* first serial (or screen) console */
int gdb_com_port = 2;			/* second serial for gdb */

/* This is set if RB_SERIAL is passed by the FreeBSD bootblocks */
int serial_console = 0;			/* set to 1 to use serial comsole, */
					/*   0 to use keyboard */
/* This is set by giving "-d" to the FreeBSD bootblocks */
int enable_gdb = 0;			/* set to 1 for kernel debugging */

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


static int 
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
direct_cons_getchar(void) 
{
	while (!(direct_cons_flags & DC_NONBLOCK) && queue_empty(&kbd_queue))
		;
	       
	return queue_remove(&kbd_queue);
}


/*
 * This overrides the _exit() function in libc.
 * If the serial console (or remote GDB) is being used, it waits
 * until all the data has cleared out of the FIFOs; if the VGA 
 * display is being used (normal console), then it waits for a keypress.
 * When it is done, it calls pc_reset() to reboot the computer.
 */
static void our_exit(int rc)
{
	printf("_exit(%d) called; rebooting...\n", rc);

	/* wait for GDB to finish receiving exit message */
	if (enable_gdb)
		com_cons_flush(gdb_com_port);

	/* wait for `_exit called` message */
	if (serial_console)
		com_cons_flush(cons_com_port);
	else {
		/* This is so that the user has a chance to SEE the output */
		printf("Press a key to reboot");
		getchar();
	}

	pc_reset();
}


/*
 * This function defines our kernel "console device";
 * calls to printf() will eventually call putchar().
 * Our implementation simply writes characters to the local PC display,
 * or the serial line, depending on the info from the bootblocks.
 */
int
console_putchar(int c)
{
	if (serial_console) {
		if (enable_gdb && (cons_com_port == gdb_com_port)) {
			gdb_serial_putchar(c);
		} else {
			com_cons_putchar(cons_com_port, c);
		}
	} else {
		direct_cons_putchar(c);
	}
	return (c);
}

/*
 * Here we provide the means to read a character.
 */
int
console_getchar(void)
{
	if (serial_console) {
		if (enable_gdb && (cons_com_port == gdb_com_port)) {
			return gdb_serial_getchar();
		} else {
			return com_cons_getchar(cons_com_port);
		}
	} else {
		return direct_cons_getchar();
	}
}

/*
 * While it is not necessary to override puts, this allows us to handle
 * gdb printfs much more efficiently by packaging them together.
 * If we haven't enabled the debugger, we just spit them all out.
 */
int
console_puts(const char *s)
{
	if (serial_console && enable_gdb && (cons_com_port == gdb_com_port))
		gdb_serial_puts(s);
	else {
		base_critical_enter();
		while (*s)
			putchar(*s++);
		putchar('\n');
		base_critical_leave();
	}
	return 0;
}

/*
 * This is more efficient for console output, and allows similar treatment
 * in usermode where character based output is really bad.
 */
int
console_putbytes(const char *s, int len)
{
	base_critical_enter();
	while (len) {
		console_putchar(*s++);
		len--;
	}
	base_critical_leave();
	return 0;
}

/*
 * This function is called at exit time if we're being debugged remotely;
 * it notifies the remoted debugger that we're exiting.
 * Unfortunately we can't pass back the true return code
 * because C's standard atexit mechanism doesn't provide it.
 */
static void gdb_atexit(void)
{
	if (enable_gdb)
		gdb_serial_exit(0);
}

/*
 * This function parses the multiboot command line and
 * initializes the serial lines.
 */
void
base_console_init(int argc, char **argv)
{
	int i;
	char *p;

	/*
	 * XXX: "-f" is a Utah-specific hack to allow FreeBSD bootblocks 
	 * to tell the kernel to run at 115200 instead of the default 9600.
	 * Note: if -f is passed w/o -h, will use the keyboard.
	 * This is done so that "-f" can be hardcoded, and just
	 * change -h to select serial/keyboard.
	 */

	/* Initialize our configuration options from environment variables */
	if ((p = getenv("CONS_COM")) != NULL) {
		cons_com_port = atoi(p);
		serial_console = 1;
	}
	if ((p = getenv("GDB_COM")) != NULL) {
		gdb_com_port = atoi(p);
		enable_gdb = 1;
	}
	if ((p = getenv("BAUD")) != NULL) {
		base_cooked_termios.c_ispeed = atoi(p);
		base_cooked_termios.c_ospeed = atoi(p);
		base_raw_termios.c_ispeed = atoi(p);
		base_raw_termios.c_ospeed = atoi(p);
	}

	/* Deal with any boot flags that we care about */
	for (i = 0; i < argc; i++) {
		if (strcmp(argv[i], "-f") == 0) {
			base_cooked_termios.c_ispeed = B115200;
			base_cooked_termios.c_ospeed = B115200;
			base_raw_termios.c_ispeed = B115200;	/* gdb */
			base_raw_termios.c_ospeed = B115200;
		} else if (strcmp(argv[i], "-h") == 0) {
			/* RB_SERIAL console flag from the BSD boot blocks */
			serial_console = 1;
		} else if (strcmp(argv[i], "-d") == 0) {
			/* enable gdb/gdb */
			enable_gdb = 1;
		}
	}

	/* Initialize the serial console, if we are using it */
	if (serial_console)
		com_cons_init(cons_com_port, &base_cooked_termios);

	/* XXX always initialize the console, just in case someone 
	 * actually tries to use it.
	 */
	osenv_timer_init();

	intr_cons_init();

	/* Initialize GDB if it is enabled */
	if (enable_gdb) {
		extern int main(int argc, char **argv);

		/*
		 * Initialize the GDB stub to use the specified COM port.
		 */
		gdb_pc_com_init(gdb_com_port, &base_raw_termios);

		/*
		 * Detach from the remote GDB when we exit.
		 */
		atexit(gdb_atexit);

#ifdef HAVE_DEBUG_REGS
		/*
		 * Set up the debug registers
		 * to catch null pointer references,
		 * and to take a breakpoint at the entrypoint to main().
		 */
		set_b0(NULL, DR7_LEN_1, DR7_RW_INST);
		set_b1(NULL, DR7_LEN_4, DR7_RW_DATA);
		set_b2((unsigned)main, DR7_LEN_1, DR7_RW_INST);
#else
		/*
		 * Take an immediate breakpoint trap here
		 * so that the debugger can take control from the start.
		 */
		asm("int $3");	/* XXX */
#endif

		/*
		 * The Intel Pentium manual recommends
		 * executing an LGDT instruction
		 * after modifying breakpoint registers,
		 * and experience shows that this is necessary.
		 */
		base_gdt_load();
	}
}

