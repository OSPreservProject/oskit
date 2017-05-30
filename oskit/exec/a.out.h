/*
 * Copyright (c) 1994, 1998 University of Utah and the Flux Group.
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
#ifndef _OSKIT_EXEC_A_OUT_
#define _OSKIT_EXEC_A_OUT_

struct exec
{
	unsigned long  	a_magic;        /* Magic number */
	unsigned long   a_text;         /* Size of text segment */
	unsigned long   a_data;         /* Size of initialized data */
	unsigned long   a_bss;          /* Size of uninitialized data */
	unsigned long   a_syms;         /* Size of symbol table */
	unsigned long   a_entry;        /* Entry point */
	unsigned long   a_trsize;       /* Size of text relocation */
	unsigned long   a_drsize;       /* Size of data relocation */
};

struct nlist {
	long		n_strx;		/* Offset of name in string table */
	unsigned char	n_type;		/* Symbol/relocation type */
	char		n_other;	/* Miscellaneous info */
	short		n_desc;		/* Miscellaneous info */
	unsigned long	n_value;	/* Symbol value */
};

#define OMAGIC 0407
#define NMAGIC 0410
#define ZMAGIC 0413
#define QMAGIC 0314

#define N_GETMAGIC(ex) \
	( (ex).a_magic & 0xffff )
#define N_GETMAGIC_NET(ex) \
	(ntohl((ex).a_magic) & 0xffff)

/* Valid magic number check. */
#define	N_BADMAG(ex) \
	(N_GETMAGIC(ex) != OMAGIC && N_GETMAGIC(ex) != NMAGIC && \
	 N_GETMAGIC(ex) != ZMAGIC && N_GETMAGIC(ex) != QMAGIC && \
	 N_GETMAGIC_NET(ex) != OMAGIC && N_GETMAGIC_NET(ex) != NMAGIC && \
	 N_GETMAGIC_NET(ex) != ZMAGIC && N_GETMAGIC_NET(ex) != QMAGIC)

/*
 * We don't provide any N_???OFF macros here
 * because they vary too much between the different a.out variants;
 * it's practically impossible to create one set of macros
 * that works for UX, FreeBSD, NetBSD, Linux, etc.
 */

#endif /* _OSKIT_EXEC_A_OUT_ */
