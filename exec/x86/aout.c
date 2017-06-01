/*
 * Copyright (c) 1993, 1998 University of Utah and the Flux Group.
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
 * Mach Operating System
 * Copyright (c) 1993,1989 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 * i386-specific routines for loading a.out files.
 */

#include <oskit/exec/a.out.h>
#include <oskit/exec/exec.h>

/* See below...
   Must be a power of two.
   This should be kept small, because we may be running on a small stack,
   and this code is not likely to be performance-critical anyway.  */
#define SCAN_CHUNK	256

int exec_load_aout(exec_read_func_t *read, exec_read_exec_func_t *read_exec,
		   void *handle, exec_info_t *out_info)
{
	struct exec	x;
	oskit_size_t	actual;
	oskit_addr_t	text_start;	/* text start in memory */
	oskit_size_t	text_size;
	oskit_addr_t	text_offset;	/* text offset in file */
	oskit_size_t	data_size;
	int		err;

	/* Read the exec header.  */
	err = (*read)(handle, 0, &x, sizeof(x), &actual);
	if (err)
		return err;
	if (actual != sizeof(x))
		return EX_NOT_EXECUTABLE;

	/*printf("get_loader_info: magic %04o\n", (int)x.a_magic);*/

	switch ((int)x.a_magic & 0xFFFF) {

	    case OMAGIC:
		text_start  = 0;
		text_size   = 0;
		text_offset = sizeof(struct exec);
		data_size   = x.a_text + x.a_data;
		break;

	    case NMAGIC:
		text_start  = 0;
		text_size   = x.a_text;
		text_offset = sizeof(struct exec);
		data_size   = x.a_data;
		break;

	    case ZMAGIC:
	    {
	    	char buf[SCAN_CHUNK];

		/* This kludge is not for the faint-of-heart...
		   Basically we're trying to sniff out the beginning of the text segment.
		   We assume that the first nonzero byte is the first byte of code,
		   and that x.a_entry is the virtual address of that first byte.  */
		for (text_offset = 0; ; text_offset++)
		{
			if ((text_offset & (SCAN_CHUNK-1)) == 0)
			{
				err = (*read)(handle, text_offset, buf,
					      SCAN_CHUNK, &actual);
				if (err)
					return err;
				if (actual < SCAN_CHUNK)
					buf[actual] = 0xff; /* ensure termination */
				if (text_offset == 0)
					text_offset = sizeof(struct exec);
			}
			if (buf[text_offset & (SCAN_CHUNK-1)])
				break;
		}

		/* Account for the (unlikely) event that the first instruction
		   is actually an add instruction with a zero opcode.
		   Surely every a.out variant should be sensible enough at least
		   to align the text segment on a 32-byte boundary...  */
		text_offset &= ~0x1f;

		text_start = x.a_entry;
		text_size = x.a_text;
		data_size   = x.a_data;
		break;
	    }

	    case QMAGIC:
		text_start	= 0x1000;
		text_offset	= 0;
		text_size	= x.a_text;
		data_size	= x.a_data;
		break;

	    default:
		/* Check for NetBSD big-endian ZMAGIC executable */
		if ((int)x.a_magic == 0x0b018600) {
			text_start  = 0x1000;
			text_size   = x.a_text;
			text_offset = 0;
			data_size   = x.a_data;
			break;
		}
		return (EX_NOT_EXECUTABLE);
	}

	/* If the text segment overlaps the same page as the beginning of the data segment,
	   then cut the text segment short and grow the data segment appropriately.  */
	if ((text_start + text_size) & 0xfff)
	{
		oskit_size_t incr = (text_start + text_size) & 0xfff;
		if (incr > text_size) incr = text_size;
		text_size -= incr;
		data_size += incr;
	}

	/*printf("exec_load_aout: text_start %08x text_offset %08x text_size %08x data_size %08x\n",
		text_start, text_offset, text_size, data_size);*/

	/* Load the read-only text segment, if any.  */
	if (text_size > 0)
	{
		err = (*read_exec)(handle, text_offset, text_size,
				   text_start, text_size,
				   EXEC_SECTYPE_READ |
				   EXEC_SECTYPE_EXECUTE |
				   EXEC_SECTYPE_ALLOC |
				   EXEC_SECTYPE_LOAD);
		if (err)
			return err;
	}

	/* Load the read-write data segment, if any.  */
	if (data_size + x.a_bss > 0)
	{
		err = (*read_exec)(handle,
				   text_offset + text_size,
				   data_size,
				   text_start + text_size,
				   data_size + x.a_bss,
				   EXEC_SECTYPE_READ |
				   EXEC_SECTYPE_WRITE |
				   EXEC_SECTYPE_EXECUTE |
				   EXEC_SECTYPE_ALLOC |
				   EXEC_SECTYPE_LOAD);
		if (err)
			return err;
	}

	/*
	 * Load the symbol table, if any.
	 * First the symtab, then the strtab.
	 */
	if (x.a_syms > 0)
	{
		unsigned strtabsize;

		/* Load the symtab. */
		err = (*read_exec)(handle,
				   text_offset + text_size + data_size,
				   x.a_syms,
				   0, 0,
				   EXEC_SECTYPE_AOUT_SYMTAB);
		if (err)
			return err;

		/*
		 * Figure out size of strtab.
		 * The size is the first word and includes itself.
		 * If there is no strtab, this file is bogus.
		 */
		err = (*read)(handle,
			      text_offset + text_size + data_size + x.a_syms,
			      &strtabsize, sizeof(strtabsize),
			      &actual);
		if (err)
			return err;
		if (actual != sizeof(strtabsize))
			return EX_CORRUPT;

		/* Load the strtab. */
		err = (*read_exec)(handle,
				   text_offset + text_size + data_size + x.a_syms,
				   strtabsize,
				   0, 0,
				   EXEC_SECTYPE_AOUT_STRTAB);
		if (err)
			return err;
	}

	out_info->entry = x.a_entry;

	return(0);
}
