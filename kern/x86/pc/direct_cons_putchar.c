/*
 * Copyright (c) 1994-1999 University of Utah and the Flux Group.
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

/* This is a special "feature" (read: kludge)
   intended for use only for kernel debugging.
   It enables an extremely simple console output mechanism
   that sends text straight to CGA/EGA/VGA video memory.
   It has the nice property of being functional right from the start,
   so it can be used to debug things that happen very early
   before any devices are initialized.  */

#include <oskit/c/string.h>
#include <oskit/base_critical.h>
#include <oskit/x86/base_vm.h>
#include <oskit/x86/pio.h>
#include <oskit/x86/pc/direct_cons.h>

int direct_cons_flags;

volatile unsigned short *vidbase;
unsigned short video_port_reg;
#define video_port_val (video_port_reg + 1)

#define MONO_BUF ((void *)phystokv(0xb0000))
#define MONO_BASE 0x3b4
#define CGA_BUF ((void *)phystokv(0xb8000))
#define CGA_BASE 0x3d4

#define ADM5 1			/* no mere adm3a, this */
#define ROWS 25			/* The world's only 25-line ADM5! */
#define COLS 80

static unsigned short cons_color = 0x0700;

static void
set_cursor(unsigned int pos)
{
	/* set cursor position high byte */
	outb_p(video_port_reg, 0xe);
	outb_p(video_port_val, (unsigned char)((pos >> 8)) & 0xff);

	/* set cursor position low byte */
        outb_p(video_port_reg, 0xf);
        outb_p(video_port_val, (unsigned char)(pos & 0xff));
}

#define clear_to_eol(ofs) clear(ofs, COLS - (ofs % COLS));
#define clear_to_eos(ofs) clear(ofs, ROWS*COLS - ofs);
static void
clear(unsigned int pos, oskit_size_t cnt)
{
	volatile unsigned short *p = &vidbase[pos];
	while (cnt--)
		*p++ = (unsigned short) 0x0f00;
}

static void
scroll(void)
{
	memcpy((void *) vidbase, (void *) (vidbase + COLS), COLS*ROWS*2);
	clear((ROWS - 1) * COLS, COLS);
}

int
direct_cons_putchar(int c)
{
	static int ofs = -1;
	static enum { normal, got_esc, got_eq, got_y } state;
	static unsigned int pending_y;

	base_critical_enter();

	if (ofs < 0)
	{
		/* Called for the first time - initialize.  */
		volatile unsigned short *p = (unsigned short *)CGA_BUF;
		unsigned short val;

		val = *p;
		*p = (unsigned short) 0xa55a;
		if (*p != 0xa55a) {
			vidbase = MONO_BUF;
			video_port_reg = MONO_BASE;
		} else {
			*p = val;
			vidbase = CGA_BUF;
			video_port_reg = CGA_BASE;
		}

		/*
		 * Figure out where the cursor is now, and leave it there
		 * if it looks reasonable.
		 */
		outb_p(video_port_reg, 0xe); /* high byte */
		val = (inb_p(video_port_val) & 0xff) << 8;
		outb_p(video_port_reg, 0xf); /* low byte */
		val |= inb_p(video_port_val) & 0xff;
		if (val < (ROWS - 1) * COLS) { /* cursor position sane? */
			if (val % COLS != 0) /* if not at start of line */
				val += COLS - (val % COLS); /* next line */
			ofs = val; /* use that position */
			clear_to_eos(val); /* clear rest just in case */
		}
		else {		/* cursor position makes no sense */
			ofs = (ROWS - 1) * COLS; /* go to last line */
			scroll(); /* scroll previous contents up */
		}
	}

	switch (state)
	{
	case normal:
		break;
	case got_esc:
		switch (c) {
		case '=':
			state = got_eq;
			goto out;
#ifdef ADM5	/*
		 * These 14 lines are the difference
		 * between an ADM3a and an ADM5.
		 */
		case 'T':	/* ESC T clear to end of line */
			clear_to_eos(ofs);
			state = normal;
			goto out;
		case 'Y':	/* ESC Y clear to end of screen */
			clear_to_eos(ofs);
			state = normal;
			goto out;
		case 'G':	/* ESC G toggle reverse video */
			cons_color = ((cons_color & 0x88ff) |
				      ((cons_color << 4) & 0x7000) |
				      ((cons_color >> 4) & 0x0700));
			c = ' '; /* print magic cookie */
			/* FALLTHROUGH */
#endif
		default:	/* bogus escape code */
			state = normal;	/* just print the character */
		}
		break;
	case got_eq:
		pending_y = c - ' ';
		state = got_y;
		goto out;
	case got_y:
		ofs = pending_y * COLS + (c - ' ');
		state = normal;
		goto repos;
	}

	switch (c)
	{
	case '\n':
		if (ofs < (ROWS - 1) * COLS)
			ofs += COLS;
		else
			scroll();
		if (direct_cons_flags & DC_NO_ONLCR)
			break;
		/* fall through... */
	case '\r':
		ofs -= ofs % COLS;
		break;
	case '\b':
		if (ofs > 0) ofs--;
		break;
	case '\t':
		do {
			direct_cons_putchar(' ');
		} while (((ofs % COLS) & 7) != 0);
		break;

	case 'Z'-'@':		/* ^Z clear screen */
		clear(0, ROWS*COLS);
		/* FALLTHROUGH */
	case '^'-'@':		/* ^^ cursor home */
		ofs = 0;
		break;
	case 'K'-'@':		/* ^K cursor up */
		ofs -= COLS;
		if (ofs < 0)
			ofs = 0;
		break;
	case '\f':		/* ^L cursor right */
		ofs++;
		break;
	case 033:		/* ESC begins cursor motion sequence */
		state = got_esc;
		goto out;

	case '\a':		/* ^G bell */
		direct_cons_bell();
		break;

	default:
		/* Scroll if we reach the end of the screen.  */
		if (ofs == ROWS*COLS) {
			scroll();
			ofs = (ROWS - 1) * COLS;
		}

		/* Stuff the character into the video buffer. */
		vidbase[ofs++] = cons_color | (unsigned char) c;
		break;
	}

 repos:
	set_cursor(ofs);

 out:
	base_critical_leave();

	return c;
}
