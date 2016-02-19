/*
 * Copyright (c) 1994-1995, 1998, 1999 University of Utah and the Flux Group.
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

#include <oskit/base_critical.h>
#include <oskit/x86/pio.h>
#include <oskit/x86/pc/direct_cons.h>
#include <oskit/x86/pc/keyboard.h>

#define SHIFT	-1
#define CTRL	-2
#define META	-3

int direct_cons_flags = 0;

static char keymap[128][2] = {
	{0},			/* 0 */
	{27,	27},		/* 1 - ESC */
	{'1',	'!'},		/* 2 */
	{'2',	'@'},
	{'3',	'#'},
	{'4',	'$'},
	{'5',	'%'},
	{'6',	'^'},
	{'7',	'&'},
	{'8',	'*'},
	{'9',	'('},
	{'0',	')'},
	{'-',	'_'},
	{'=',	'+'},
	{8,	8},		/* 14 - Backspace */
	{'\t',	'\t'},		/* 15 */
	{'q',	'Q'},
	{'w',	'W'},
	{'e',	'E'},
	{'r',	'R'},
	{'t',	'T'},
	{'y',	'Y'},
	{'u',	'U'},
	{'i',	'I'},
	{'o',	'O'},
	{'p',	'P'},
	{'[',	'{'},
	{']',	'}'},		/* 27 */
	{'\r',	'\r'},		/* 28 - Enter */
	{CTRL,	CTRL},		/* 29 - Ctrl */
	{'a',	'A'},		/* 30 */
	{'s',	'S'},
	{'d',	'D'},
	{'f',	'F'},
	{'g',	'G'},
	{'h',	'H'},
	{'j',	'J'},
	{'k',	'K'},
	{'l',	'L'},
	{';',	':'},
	{'\'',	'"'},		/* 40 */
	{'`',	'~'},		/* 41 */
	{SHIFT,	SHIFT},		/* 42 - Left Shift */
	{'\\',	'|'},		/* 43 */
	{'z',	'Z'},		/* 44 */
	{'x',	'X'},
	{'c',	'C'},
	{'v',	'V'},
	{'b',	'B'},
	{'n',	'N'},
	{'m',	'M'},
	{',',	'<'},
	{'.',	'>'},
	{'/',	'?'},		/* 53 */
	{SHIFT,	SHIFT},		/* 54 - Right Shift */
	{0,	0},		/* 55 - Print Screen */
	{META,	META},		/* 56 - Alt */
	{' ',	' '},		/* 57 - Space bar */
	{0,	0},		/* 58 - Caps Lock */
	{0,	0},		/* 59 - F1 */
	{0,	0},		/* 60 - F2 */
	{0,	0},		/* 61 - F3 */
	{0,	0},		/* 62 - F4 */
	{0,	0},		/* 63 - F5 */
	{0,	0},		/* 64 - F6 */
	{0,	0},		/* 65 - F7 */
	{0,	0},		/* 66 - F8 */
	{0,	0},		/* 67 - F9 */
	{0,	0},		/* 68 - F10 */
	{0,	0},		/* 69 - Num Lock */
	{0,	0},		/* 70 - Scroll Lock */
	{'7',	'7'},		/* 71 - Numeric keypad 7 */
	{'8',	'8'},		/* 72 - Numeric keypad 8 */
	{'9',	'9'},		/* 73 - Numeric keypad 9 */
	{'-',	'-'},		/* 74 - Numeric keypad '-' */
	{'4',	'4'},		/* 75 - Numeric keypad 4 */
	{'5',	'5'},		/* 76 - Numeric keypad 5 */
	{'6',	'6'},		/* 77 - Numeric keypad 6 */
	{'+',	'+'},		/* 78 - Numeric keypad '+' */
	{'1',	'1'},		/* 79 - Numeric keypad 1 */
	{'2',	'2'},		/* 80 - Numeric keypad 2 */
	{'3',	'3'},		/* 81 - Numeric keypad 3 */
	{'0',	'0'},		/* 82 - Numeric keypad 0 */
	{'.',	'.'},		/* 83 - Numeric keypad '.' */
};

#if 0	/* this timing is CPU specific */
/*
 * Empirically determined delay value sufficient to accomodate scan codes
 * in-transit between the keyboard and PC interface.  Value is the maximum
 * number of polls to the keyboard status register.
 */
static int direct_cons_delay_value = 4000;	/* ~4-5 ms */

void
direct_cons_delay(void)
{
	int i = direct_cons_delay_value;

	while ((inb(0x64) & K_OBUF_FUL) == 0 && --i >= 0)
		;
}

/*
 * Throw away any buffered input characters.
 * We need to delay between polls to allow time for characters to be
 * transfered between the keyboard and controller.
 */
void
direct_cons_flush_input(void)
{
	int ch;

	do {
		direct_cons_delay();
		ch = direct_cons_trygetchar();
	} while (ch >= 0);
}
#endif

/*
 * Quick poll for a pending input character.
 * Returns a character if available, -1 otherwise.  This routine can return
 * false negatives in the following cases:
 *
 *	- a valid character is in transit from the keyboard when called
 *	- a key release is received (from a previous key press)
 *	- a SHIFT key press is received (shift state is recorded however)
 *	- a key press for a multi-character sequence is received
 *
 * Yes, this is horrible.
 */
int
direct_cons_trygetchar(void)
{
	static unsigned shift_state, ctrl_state, meta_state;
	unsigned scan_code, ch;

	base_critical_enter();

	/* See if a scan code is ready, returning if none. */
	if ((inb(K_STATUS) & K_OBUF_FUL) == 0) {
		base_critical_leave();
		return -1;
	}
	scan_code = inb(K_RDWR);

	/* We just want raw scancodes; no translation for us! */
	if (direct_cons_flags & DC_RAW) {
		base_critical_leave();
		return scan_code;
	}

	/* Handle key releases - only release of SHIFT is important. */
	if (scan_code & 0x80) {
		scan_code &= 0x7f;
		if (keymap[scan_code][0] == SHIFT)
			shift_state = 0;
		else if (keymap[scan_code][0] == CTRL)
			ctrl_state = 0;
		else if (keymap[scan_code][0] == META)
			meta_state = 0;
		ch = -1;
	} else {
		/* Translate the character through the keymap. */
		ch = keymap[scan_code][shift_state] | meta_state;
		if (ch == SHIFT) {
			shift_state = 1;
			ch = -1;
		} else if (ch == CTRL) {
			ctrl_state = 1;
			ch = -1;
		} else if (ch == META) {
			meta_state = 0200;
			ch = -1;
		} else if (ch == 0)
			ch = -1;
		else if (ctrl_state)
			ch = (keymap[scan_code][1] - '@') | meta_state;
	}

	base_critical_leave();

	return ch;
}

void
direct_cons_set_flags(int flags)
{
	direct_cons_flags = flags;
}
