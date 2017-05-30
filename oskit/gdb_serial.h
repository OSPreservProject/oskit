/*
 * Copyright (c) 1994-1996 Sleepless Software
 * Copyright (c) 1997-1998 University of Utah and the Flux Group.
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
#ifndef _OSKIT_GDB_SERIAL_H_
#define _OSKIT_GDB_SERIAL_H_

#include <oskit/compiler.h>
#include <oskit/machine/types.h>

/*
 * This is actually defined in <oskit/machine/gdb.h>,
 * but we don't actually need the full definition in this header file.
 */
struct gdb_state;

OSKIT_BEGIN_DECLS

/*
 * Parameter block used by `gdb_serial_converse' and its callbacks.
 */
struct gdb_params
{
	/*
	 * An ID number for the current thread, -1 if no thread callbacks.
	 */
	oskit_addr_t thread_id;

	/*
	 * The signal number describing why the program stopped.
	 * On GDB_CONT return, signal to take when continuing or zero
	 * to continue execution normally.
	 */
	oskit_u8_t signo;

	/*
	 * Register state of the current thread.
	 * The caller must provide this buffer.
	 */
	struct gdb_state *regs;

	/*
	 * Set by `gdb_serial_converse' when values in `regs' have been
	 * changed.  The caller (or callback) then needs to commit these
	 * changes to the thread.
	 */
	int regs_changed;
};

/*
 * Return values from `gdb_serial_converse' (first one is never returned).
 * These indicate to the caller what the user has asked GDB to do.
 */
enum gdb_action {
	GDB_MORE,		/* internal - keeping reading gdb cmds */
	GDB_CONT,		/* continue with signo */
	GDB_RUN,		/* rerun the program from the beginning */
	GDB_KILL,		/* kill the program */
	GDB_DETACH		/* continue; debugger is going away */
};

/*
 * Structure of callbacks for `gdb_serial_converse'.
 * Any of these pointers may be null, indicating the facility is not available.
 */
struct gdb_callbacks
{
	/*
	 * Return zero iff `thread_id' matches a live thread.
	 */
	int (*thread_alive) (struct gdb_params *p, oskit_addr_t thread_id);

	/*
	 * Called only if `thread_id' differs from `p->thread_id'.
	 * If `p->regs_changed' is set, the now-current thread needs
	 * its register state sync'd with `p->regs'.  After that,
	 * switch to `thread_id' and fetch its register state into `p->regs'.
	 * Return zero if successful or an error code if `thread_id' is bogus.
	 */
	int (*thread_switch) (struct gdb_params *p, oskit_addr_t thread_id);
};

/*
 * These function pointers define how we send and receive characters
 * over the serial line.  They must be initialized before gdb_serial_stub()
 * is called for the first time.
 */
extern int (*gdb_serial_recv)(void);
extern void (*gdb_serial_send)(int ch);

/*
 * Fancy entry-point for the GDB serial-line stub; see above.
 * The `cb' pointer may be null if no callbacks are supported.
 */
enum gdb_action gdb_serial_converse (struct gdb_params *p,
				     const struct gdb_callbacks *cb);

/*
 * This is the main part of the GDB stub for serial-line remote debugging.
 * Call it with a signal number indicating the signal
 * that caused the program to stop,
 * and a snapshot of the program's register state.
 * After this routine returns, if *inout_signo has been set to 0,
 * the program's execution should be resumed as if nothing had happened.
 * If *inout_signo is nonzero,
 * then that signal should be delivered to the program
 * as if the program had caused the signal itself.
 * The register state in *inout_state may have been changed;
 * the new register state should be used when resuming execution.
 */
void gdb_serial_signal(int *inout_signo, struct gdb_state *inout_state);

/*
 * Call this routine to indicate that the program is exiting normally.
 * The 'exit_code' specifies the exit code sent back to GDB.
 * This function will cause the GDB session to be broken;
 * the caller can then reboot or exit or whatever locally.
 */
void gdb_serial_exit(int exit_code);

/*
 * Same, but tells GDB the program died abnormally with the given signal.
 */
void gdb_serial_killed(int signo);

/*
 * Call these routines to output characters or strings
 * to the remote debugger's standard output.
 */
void gdb_serial_putchar(int ch);
void gdb_serial_puts(const char *s);
int gdb_serial_getchar();
OSKIT_END_DECLS

#endif /* _OSKIT_GDB_SERIAL_H_ */
