/*
 * Copyright (c) 1994-1996 Sleepless Software
 * Copyright (c) 1997-1999 University of Utah and the Flux Group.
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
 * Remote serial-line source-level debugging for the Flux OS Toolkit.
 *	Loosely based on i386-stub.c from GDB 4.14
 */

#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <oskit/gdb.h>
#include <oskit/gdb_serial.h>
#include <oskit/base_critical.h>
#include <oskit/machine/types.h>
#include <oskit/machine/gdb.h>
#include <stdlib.h>


/* BUFMAX defines the maximum number of characters in inbound/outbound buffers*/
/* at least NUMREGBYTES*2 are needed for register packets */
#define BUFMAX 400


/* These function pointers define how we send and receive characters
   over the serial line.  */
int (*gdb_serial_recv)(void);
void (*gdb_serial_send)(int ch);

/* True if we have an active connection to a remote debugger.
   This is used to avoid sending 'O' (console output) messages
   before the connection has been established or after it is closed,
   which tends to hang us and/or confuse the debugger.  */
static int connected;

int gdb_serial_debug;

static const char hexchars[]="0123456789abcdef";

static int hex(char ch)
{
	if ((ch >= '0') && (ch <= '9')) return (ch-'0');
	if ((ch >= 'a') && (ch <= 'f')) return (ch-'a'+10);
	if ((ch >= 'A') && (ch <= 'F')) return (ch-'A'+10);
	return (-1);
}

static int hex2bin(char *in, unsigned char *out, int len)
{
	if (gdb_serial_debug) printf("hex2bin %d chars: %.*s\n", len, len, in);
	while (len--)
	{
		int ch = (hex(in[0]) << 4) | hex(in[1]);
		if (ch < 0)
			return -1;
		*out++ = ch;
		in += 2;
	}
	return 0;
}

/* Scan for the sequence $<data>#<checksum> */
static void getpacket(char *buffer)
{
	unsigned char checksum;
	unsigned char xmitcsum;
	int  i;
	int  count;
	char ch;

	do {
		/* Wait around for the start character;
		   ignore all other characters.  */
		while ((ch = (gdb_serial_recv() & 0x7f)) != '$');
		checksum = 0;
		xmitcsum = -1;

		count = 0;

		/* Now, read until a # or end of buffer is found.  */
		while (count < BUFMAX) {
			ch = gdb_serial_recv();

			if ((ch == '#') && 
			    (count == 0 || buffer[count-1] != 0x7d))
				break;

			checksum = checksum + ch;
			buffer[count] = ch;
			count = count + 1;
		}
		buffer[count] = 0;

		if (ch == '#')
		{
			xmitcsum = hex(gdb_serial_recv()) << 4;
			xmitcsum += hex(gdb_serial_recv());
			if (checksum != xmitcsum) 
				gdb_serial_send('-'); /* failed checksum */
			else
			{
				gdb_serial_send('+'); /* successful transfer */

				/* If a sequence char is present,
				   reply the sequence ID.  */
				if (buffer[2] == ':')
				{
					gdb_serial_send( buffer[0] );
					gdb_serial_send( buffer[1] );

					/* remove sequence chars from buffer */
					count = strlen(buffer);
					for (i=3; i <= count; i++)
						buffer[i-3] = buffer[i];
				}
			}
		}
	} while (checksum != xmitcsum);
}

struct outfrag {
	const unsigned char *data;
	unsigned int len;
	int hexify;
};

static void
putfrags (const struct outfrag *frags, unsigned int nfrags)
{
	/*  $<packet info>#<checksum>. */
	do {
		unsigned char checksum = 0;
#define SEND(ch) do { gdb_serial_send(ch); checksum += (ch); } while (0)
		const struct outfrag *f;

		/*
		 * Packets begin with '$'.
		 */
		gdb_serial_send('$');

		for (f = frags; f < &frags[nfrags]; ++f) {
			const unsigned char *p = f->data;

			while (f->len ? (p < f->data + f->len) :
			       (*p != '\0')) {
				/*
				 * See if we have a run of more than three
				 * of the same output character.
				 */
				if ((f->len == 0 ||
				     &p[f->hexify ? 1 : 3] < f->data + f->len)
				    && p[1] == p[0] && /* Two equal bytes. */
				    (f->hexify
				     /*
				      * When hexifying, each input byte
				      * turns into two output bytes, so we
				      * will have four output bytes if the
				      * two nibbles of this byte match.
				      */
				     ? ((*p >> 4) == (*p & 0xf))
				     : (p[2] == *p && p[3] == *p))) {
					/*
					 * Run-length encode sequences of
					 * four or more repeated characters.
					 */
					unsigned const char *q, *limit;
					unsigned int rl;
					if (f->hexify)
						q = &p[2], limit = &p[98 / 2];
					else
						q = &p[4], limit = &p[98];
					if (f->len && limit > f->data + f->len)
						limit = f->data + f->len;
					while (q < limit && *q == *p)
						++q;
					if (f->hexify) {
						rl = (q - p) * 2;
						SEND(hexchars[*p & 0xf]);
					}
					else {
						rl = q - p;
						SEND(*p);
					}
					SEND('*');
					SEND(29 + rl - 1);
					p = q;
				}
				else if (f->hexify) {
					SEND(hexchars[*p >> 4]);
					SEND(hexchars[*p & 0xf]);
					++p;
				}
				else {
					/*
					 * Otherwise, just send
					 * the plain character.
					 */
					SEND(*p);
					++p;
				}
			}
		}

		/*
		 * Packet data is followed by '#' and two hex digits
		 * of checksum.
		 */
		gdb_serial_send('#');
		gdb_serial_send(hexchars[checksum >> 4]);
		gdb_serial_send(hexchars[checksum % 16]);

		/*
		 * Repeat the whole packet until gdb likes it.
		 */
	} while ((gdb_serial_recv() & 0x7f) != '+');
}
#undef SEND

#define putpacket(frag...) do {						      \
	struct outfrag frags[] = { frag };				      \
	putfrags (frags, sizeof frags / sizeof frags[0]);		      \
} while (0)

enum gdb_action
gdb_serial_converse (struct gdb_params *p, const struct gdb_callbacks *cb)
{
	char inbuf[BUFMAX+1];
	unsigned char errchar;
#define PUTERR(code)	do { errchar = (code); goto errpacket; } while (0)
	enum gdb_action retval = GDB_MORE;

	p->regs_changed = 0;	/* set this when we mutate P->regs */

	base_critical_enter();

	/*
	 * Start with a spontaneous report to GDB of the arriving signal.
	 */
	goto report;

	do {
		/* Wait for a command from on high */
		getpacket(inbuf);

		/* OK, now we're officially connected */
		connected = 1;

		switch (inbuf[0]) {
		case 'H':	/* set thread for following operations */
		{
			oskit_addr_t th;
			char *endth;

			th = strtol (&inbuf[2], &endth, 16);
			if (endth == &inbuf[2])
				PUTERR(4);

			if (!cb || !cb->thread_switch)
				/*
				 * If we have no real awareness of threads,
				 * make no pretense of it.
				 */
				goto unknown;

			if (th != -1 && th != 0 && th != p->thread_id) {
				errchar = cb->thread_switch(p, th);
				if (errchar)
					goto errpacket;
				/*
				 * The callback function has filled P->regs
				 * with the new thread's register state.
				 */
				p->thread_id = th;
				p->regs_changed = 0;
			}
			goto ok;
		}

		case 'T':	/* verify thread is alive */
		{
			oskit_addr_t th;
			char *endth;

			th = strtol (&inbuf[1], &endth, 16);
			if (endth == &inbuf[1])
				PUTERR(1);

			if (cb && cb->thread_alive) {
				errchar = cb->thread_alive(p, th);
				if (errchar)
					goto errpacket;
			}
			else
				/*
				 * We have no way to check, so respond
				 * like we don't understand the request.
				 */
				goto unknown;

			goto ok; /* yes, thread is alive */
		}

		case 'g': /* return the value of the CPU registers */
		{
			/* Send it over like a memory dump.  */
			putpacket({(void*)p->regs, sizeof(*p->regs), 1});
			break;
		}

		case 'G': /* set the CPU registers - return OK */
		{
			/* Unpack the new register state.  */
			if (hex2bin(&inbuf[1], (void *) p->regs,
				    sizeof(*p->regs)))
				PUTERR(4);

			p->regs_changed = 1;
			goto ok;
		}

		case 'P':	/* set individual CPU register */
		{
			char *eqp;
			unsigned int reg;

			reg = strtoul (&inbuf[1], &eqp, 16);
			if (eqp == &inbuf[1] || *eqp != '=')
				PUTERR(4);

			/*
			 * Check for bogus register number.
			 */
			if ((reg + 1) * sizeof(oskit_addr_t) > sizeof *p->regs
			    /*
			     * Convert the value and modify the register state.
			     */
			    || hex2bin (eqp + 1,
					(void *) &((oskit_addr_t *) p->regs)[reg],
					sizeof (oskit_addr_t)))
				PUTERR(4);

			/*
			 * Mark that the register state needs to be
			 * stored back into the thread before it continues.
			 */
			p->regs_changed = 1;
			goto ok;
		}

		/* mAA..AA,LLLL  Read LLLL bytes at address AA..AA */
		case 'm':
		{
			oskit_addr_t addr, len;
			char *ptr, *ptr2;

			/* Read the start address and length.
			   If invalid, send back an error code.  */
			addr = strtoul(&inbuf[1], &ptr, 16);
			if ((ptr == &inbuf[1]) || (*ptr != ','))
				PUTERR (2);
			ptr++;
			len = strtoul(ptr, &ptr2, 16);
			if ((ptr2 == ptr) || (len*2 > BUFMAX))
				PUTERR (2);

			/* Copy the data into a kernel buffer.
			   If a fault occurs, return an error code.  */
			if (gdb_copyin(addr, inbuf, len))
				PUTERR(3);

			/* Convert it to hex data on the way out.  */
			putpacket({inbuf, len, 1});
			break;
		}

		/* MAA..AA,LLLL: Write LLLL bytes at AA.AA return OK */
		case 'M' :
		{
			oskit_addr_t addr, len;
			char *ptr, *ptr2;

			/* Read the start address and length.
			   If invalid, send back an error code.  */
			addr = strtoul(&inbuf[1], &ptr, 16);
			if ((ptr == &inbuf[1]) || (*ptr != ','))
				PUTERR(2);
			ptr++;
			len = strtoul(ptr, &ptr2, 16);
			if ((ptr2 == ptr) || (*ptr2 != ':'))
				PUTERR(2);
			ptr2++;
			if (ptr2 + len*2 > &inbuf[BUFMAX])
				PUTERR(2);

			{
				char outbuf[len];
				/* Convert the hex input data to binary.  */
				if (hex2bin(ptr2, outbuf, len))
					PUTERR(2);

				/* Copy the data into its final place.
				   If a fault occurs, return an error code.  */

				if (gdb_copyout(outbuf, addr, len))
					PUTERR(3);
			}
			goto ok;
		}

		/* XAA..AA,LLLL: Write LLLL bytes at AA.AA return OK
		 * The difference from M is that the data is not
		 * hexified, but in binary with an escape char.
		 */
		case 'X':
		{
			oskit_addr_t addr, len;
			char *ptr, *ptr2;

			/* Read the start address and length.
			   If invalid, send back an error code.  */
			addr = strtoul(&inbuf[1], &ptr, 16);
			if ((ptr == &inbuf[1]) || (*ptr != ','))
				PUTERR(2);
			ptr++;
			len = strtoul(ptr, &ptr2, 16);
			if ((ptr2 == ptr) || (*ptr2 != ':'))
				PUTERR(2);
			ptr2++;
			if (ptr2 + len > &inbuf[BUFMAX])
				PUTERR(2);

			{
				/*
				 * The data is mostly binary, but with
				 * some characters quoted by a 0x7d ('}')
				 * byte indicating the following byte has
				 * been XOR'd with 0x20.  We overwrite the
				 * data buffer a byte at a time left to right
				 * with the dequoted binary contents.
				 */
				unsigned char *in, *end, *out;

				in = (unsigned char *) ptr2;
				end = in + len;
				out = (unsigned char *) inbuf;
				while (in < end) {
					if (*in == 0x7d) {
						if (end - in < 2)
							PUTERR(2);
						*out++ = in[1] ^ 0x20;
						in += 2;
					}
					else
						*out++ = *in++;
				}

				/* Copy the data into its final place.
				   If a fault occurs, return an error code.  */

				if (gdb_copyout(inbuf, addr,
						(char *) out - inbuf))
					PUTERR(3);
			}
			goto ok;
		}

		/* cAA..AA Continue at address AA..AA(optional) */
		/* sAA..AA Step one instruction from AA..AA(optional) */
		case 'c' :
		case 's' :
		{
			oskit_addr_t new_eip;
			char *ptr;

			/* Try to read optional parameter;
			   leave EIP unchanged if none.  */
			new_eip = strtoul(&inbuf[1], &ptr, 16);
			if (ptr > &inbuf[1] && new_eip != p->regs->eip) {
				p->regs->eip = new_eip;
				p->regs_changed = 1;
			}

			/* Set the trace flag appropriately.  */
			gdb_set_trace_flag(inbuf[0] == 's', p->regs);

			/* Return and "consume" the signal
			   that caused us to be called.  */
			p->signo = 0;
			retval = GDB_CONT;
			break;
		}

		/* CssAA..AA Continue with signal ss */
		/* SssAA..AA Step one instruction with signal ss */
		case 'C' :
		case 'S' :
		{
			oskit_addr_t new_eip;
			int nsig;
			char *ptr;

			/* Read the new signal number.  */
			nsig = hex(inbuf[1]) << 4 | hex(inbuf[2]);
			if ((nsig <= 0) || (nsig >= NSIG))
				PUTERR (5);

			/* Try to read optional parameter;
			   leave EIP unchanged if none.  */
			new_eip = strtoul(&inbuf[3], &ptr, 16);
			if (ptr > &inbuf[3] && new_eip != p->regs->eip) {
				p->regs->eip = new_eip;
				p->regs_changed = 1;
			}

			/* Set the trace flag appropriately.  */
			gdb_set_trace_flag(inbuf[0] == 'S', p->regs);

			/* Return and "consume" the signal
			   that caused us to be called.  */
			p->signo = nsig;
			retval = GDB_CONT;
			break;
		}

		report:
		case '?':
			if (cb && cb->thread_switch &&
			    p->thread_id != 0 && p->thread_id != -1) {
				/*
				 * Send a report including thread id.
				 */
				sprintf (inbuf, "thread:%lx;",
					 (unsigned long int) p->thread_id);
				putpacket({"T"},{&p->signo,1,1},{inbuf});
			}
			else
				putpacket({"S"},{&p->signo,1,1});
			break;

		case 'D':		/* debugger detaching from us */
			connected = 0;
			retval = GDB_DETACH;
			goto ok;

		case 'R':		/* restart the program */
			connected = 0;
			retval = GDB_RUN;
			/*
			 * Send no response packet.
			 * Once we've ACK'd receiving the restart packet,
			 * gdb tries immediately to query the restarted state.
			 */
			break;

		case 'k':		/* kill the program */
			connected = 0;
			retval = GDB_KILL;
			/*
			 * Send no response packet.
			 * Once we've ACK'd receiving the kill packet
			 * (in getpacket, above), gdb ignores us.
			 */
			break;

		case 'd':		/* toggle debug flag */
			/*
			 * This toggles a `debug flag' for this stub itself.
			 * If we ever add any debugging printfs, they should
			 * be conditional on this.
			 */
			gdb_serial_debug = ! gdb_serial_debug;
			goto ok;

#if 0
		case 't':		/* search memory */
		{
			oskit_addr_t addr, ptn, mask;
			char *ptr, *ptr2;

			/* Read the start address and length.
			   If invalid, send back an error code.  */
			addr = strtoul(&inbuf[1], &ptr, 16);
			if ((ptr == &inbuf[1]) || (*ptr != ':'))
				PUTERR(2);
			ptr++;
			ptn = strtoul(ptr, &ptr2, 16);
			if ((ptr2 == ptr) || (*ptr2 != ','))
				PUTERR(2);
			ptr2++;
			mask = strtoul(ptr2, &ptr, 16);
			if (ptr == ptr2)
				PUTERR(2);

			/*
			 * Search memory: need a new callback for this.
			 * Don't bother, since as of 970202 gdb doesn't use `t' anyway.
			 */
			XXX

				goto ok;
		}
#endif

#if 0				/* XXX */
		case 'q':		/* generic "query" request */
			/* qOffsets -> Text=%x;Data=%x;Bss=%x */
		case 'Q':		/* generic "set" request */
#endif

		case '!':
			/*
			 * This is telling us gdb wants to use the
			 * "extended" protocol.  As far as I can tell, the
			 * extended protocol is backward compatible, so
			 * this is a no-op and we always grok it.
			 */
			/* FALLTHROUGH */
		ok:
		putpacket({"OK"});
		break;

		errpacket:	/* error replies jump to here */
		putpacket({"E"},{&errchar,1,1});
		break;

		default:
		unknown:
			/*
			 * Return a blank response to a packet we don't grok.
			 */
			putpacket ({""});
			break;
		}
	} while (retval == GDB_MORE);

	base_critical_leave();

	return retval;
}

void gdb_serial_signal(int *inout_signo, struct gdb_state *state)
{
	struct gdb_params p = { -1, *inout_signo, state };
	switch (gdb_serial_converse(&p, 0)) {
	case GDB_CONT:
	case GDB_DETACH:
		*inout_signo = p.signo;
		return;
	case GDB_KILL:
	case GDB_RUN:
		panic("Program terminated by debugger");
		break;
	default:
		panic("gdb_serial_signal confused");
	}
}

void gdb_serial_exit(int exit_code)
{
	const unsigned char exchar = exit_code;

	base_critical_enter();
	if (!gdb_serial_send || !gdb_serial_recv || !connected) {
		base_critical_leave();
		return;
	}

	/* Indicate that we have terminated.  */
	putpacket({"W"},{&exchar,1,1});
	connected = 0;

	base_critical_leave();
	/* XXX: should wait until the FIFO has emptied */
}

void gdb_serial_killed(int signo)
{
	const unsigned char exchar = signo;

	base_critical_enter();
	if (!gdb_serial_send || !gdb_serial_recv || !connected) {
		base_critical_leave();
		return;
	}

	/* Indicate that we have terminated.  */
	putpacket({"X"},{&exchar,1,1});
	connected = 0;

	base_critical_leave();
}

void gdb_serial_putchar(int ch)
{
	const unsigned char uch = ch;

	base_critical_enter();
	if (!gdb_serial_send || !gdb_serial_recv) {
		base_critical_leave();
		return;
	}

	if (!connected) {
		gdb_serial_send(ch);
		base_critical_leave();
		return;
	}

	putpacket ({"O"}, {&uch,1,1});
	base_critical_leave();
}

void gdb_serial_puts(const char *s)
{
	base_critical_enter();
	if (!gdb_serial_send || !gdb_serial_recv) {
		base_critical_leave();
		return;
	}

	if (!connected) {
		while (*s) gdb_serial_send(*s++);
		gdb_serial_send('\n');
		base_critical_leave();
		return;
	}

	putpacket({"O"}, {s,0,1}, {"\n",1,1});
	base_critical_leave();
}

int gdb_serial_getchar()
{
	static char inbuf[256];
	static int pos = 0;
	int c;

	base_critical_enter();

	/*
	 * Unfortunately, the GDB protocol doesn't support console input.
	 * However, we can simulate it with a rather horrible kludge:
	 * we take a breakpoint and let the user fill the input buffer
	 * with a command like 'call strcpy(inbuf, "hello\r")'.
	 * The supplied characters will be returned
	 * from successive calls to gdb_serial_getchar(),
	 * until the inbuf is emptied,
	 * at which point we hit a breakpoint again.
	 */
	while (inbuf[pos] == 0) {
		inbuf[pos = 0] = 0;
		gdb_breakpoint();	/* input needed! */
	}				/* input needed! */
	c = inbuf[pos++];

	base_critical_leave();

	return c;
}
