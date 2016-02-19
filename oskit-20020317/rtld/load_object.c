/*
 * Copyright (c) 1999 University of Utah and the Flux Group.
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
/*-
 * Copyright 1996-1998 John D. Polstra.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *      $\Id: load_object.c,v 1.1 1999/06/29 17:37:55 stoller Exp $
 */

#ifdef OSKIT
#include <oskit/page.h>
#else
#include <sys/param.h>
#endif

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <malloc.h>

#include "rtld.h"
#include "debug.h"

void	dump_object(Obj_Entry *pobj);

Obj_Entry *
read_object(int fd)
{
	Obj_Entry	*obj;
	union {
		Elf_Ehdr hdr;
		char buf[PAGE_SIZE];
	} u;
	int		nbytes, nsegs;
	Elf_Phdr	*phdr, *phlimit, *segs[2], *phdyn, *phphdr;
	caddr_t		mapbase, objbase;
	size_t		mapsize;
	Elf_Off		base_offset;
	Elf_Addr	base_vaddr, base_vlimit;

	if ((nbytes = read(fd, u.buf, PAGE_SIZE)) == -1) {
		_rtld_error("Read error: %s", strerror(errno));
		return NULL;
	}

	/* Make sure the file is valid */
	if (nbytes < sizeof(Elf_Ehdr)
	    || u.hdr.e_ident[EI_MAG0] != ELFMAG0
	    || u.hdr.e_ident[EI_MAG1] != ELFMAG1
	    || u.hdr.e_ident[EI_MAG2] != ELFMAG2
	    || u.hdr.e_ident[EI_MAG3] != ELFMAG3) {
		_rtld_error("Invalid file format");
		return NULL;
	}
	if (u.hdr.e_ident[EI_CLASS] != ELF_TARG_CLASS
	    || u.hdr.e_ident[EI_DATA] != ELF_TARG_DATA) {
		_rtld_error("Unsupported file layout");
		return NULL;
	}
	if (u.hdr.e_ident[EI_VERSION] != EV_CURRENT
	    || u.hdr.e_version != EV_CURRENT) {
		_rtld_error("Unsupported file version");
		return NULL;
	}
	if (u.hdr.e_type != ET_EXEC && u.hdr.e_type != ET_DYN) {
		_rtld_error("Unsupported file type");
		return NULL;
	}
	if (u.hdr.e_machine != ELF_TARG_MACH) {
		_rtld_error("Unsupported machine");
		return NULL;
	}

	/*
	 * We rely on the program header being in the first page.  This is
	 * not strictly required by the ABI specification, but it seems to
	 * always true in practice.  And, it simplifies things considerably.
	 */
	assert(u.hdr.e_phentsize == sizeof(Elf_Phdr));
	assert(u.hdr.e_phoff + u.hdr.e_phnum*sizeof(Elf_Phdr) <= PAGE_SIZE);
	assert(u.hdr.e_phoff + u.hdr.e_phnum*sizeof(Elf_Phdr) <= nbytes);

	/*
	 * Scan the program header entries, and save key information.
	 *
	 * We rely on there being exactly two load segments, text and data,
	 * in that order.
	 */
	phdr = (Elf_Phdr *) (u.buf + u.hdr.e_phoff);
	phlimit = phdr + u.hdr.e_phnum;
	nsegs = 0;
	phdyn = NULL;
	phphdr = NULL;
	while (phdr < phlimit) {
		switch (phdr->p_type) {
		case PT_LOAD:
			assert(nsegs < 2);
			segs[nsegs] = phdr;
			++nsegs;
			break;

		case PT_PHDR:
			phphdr = phdr;
			break;

		case PT_DYNAMIC:
			phdyn = phdr;
			break;
		}

		++phdr;
	}
	if (phdyn == NULL) {
		_rtld_error("Object is not dynamically-linked");
		return NULL;
	}

	assert(nsegs == 2);
	assert(segs[0]->p_align >= PAGE_SIZE);
	assert(segs[1]->p_align >= PAGE_SIZE);

	/*
	 * Compute the object limits. 
	 */
	base_offset = trunc_page(segs[0]->p_offset);
	base_vaddr  = trunc_page(segs[0]->p_vaddr);
	base_vlimit = round_page(segs[1]->p_vaddr + segs[1]->p_memsz);
	mapsize     = base_vlimit - base_vaddr;
	mapsize    += PAGE_SIZE;	/* For headers */

	dbg("load_object: bo:0x%x bv:0x%x bvl:0x%x ms:0x%x\n",
	    base_offset, base_vaddr, base_vlimit, mapsize);

	/*
	 * Allocate a region of memory for the object and read the object
	 * into it. This will form the base for relocation.
	 */
	if ((mapbase = (caddr_t) xsmemalign(PAGE_SIZE, mapsize)) == NULL) {
		_rtld_error("load_object: Could not allocate %d bytes of "
			    "memory for object", mapsize);
		return NULL;
	}
	memset(mapbase, 0, mapsize);

	/*
	 * Copy the program header into the first page. The object starts
	 * in the second page.
	 */
	memcpy(mapbase, u.buf, PAGE_SIZE);
	objbase = mapbase + (PAGE_SIZE/sizeof(caddr_t));

	/*
	 * Read the text and data segments. The BSS is already cleared.
	 */
	lseek(fd, segs[0]->p_offset, SEEK_SET);
	if ((nbytes = read(fd, objbase, segs[0]->p_filesz)) == -1) {
		_rtld_error("load_object: Could not read text segment");
		sfree(mapbase, mapsize);
		return NULL;
	}

	lseek(fd, segs[1]->p_offset, SEEK_SET);
	if ((nbytes = read(fd, (caddr_t) (objbase + segs[1]->p_vaddr),
			   segs[1]->p_filesz)) == -1) {
		_rtld_error("load_object: Could not read text segment");
		sfree(mapbase, mapsize);
		return NULL;
	}

	/*
	 * Set up the Object descriptor.
	 */
	if ((obj = (Obj_Entry *) xcalloc(sizeof(Obj_Entry))) == NULL) {
		_rtld_error("load_object: "
			    "Could not allocate object descriptor");
		sfree(mapbase, mapsize);
		return NULL;
	}

	obj->mapbase   = mapbase;
	obj->mapsize   = mapsize;
	obj->textsize  = round_page(segs[0]->p_vaddr + segs[0]->p_memsz) -
		         base_vaddr;
	obj->vaddrbase = base_vaddr;
	obj->relocbase = objbase - base_vaddr;
	obj->dynamic   = (const Elf_Dyn *)
		         (objbase + (phdyn->p_vaddr - base_vaddr));
	if (u.hdr.e_entry != 0)
		obj->entry = (caddr_t)
			     (objbase + (u.hdr.e_entry - base_vaddr));
	if (phphdr != NULL) {
		obj->phdr = (const Elf_Phdr *)
			    (objbase + (phphdr->p_vaddr - base_vaddr));
		obj->phsize = phphdr->p_memsz;
	}

#ifdef RTLDDEBUG
	dump_object(obj);
#endif
	return obj;
}

/*
 * Load headers for the main program.
 */
Elf_Phdr *
load_headers(char *name, int *phnum, caddr_t *entry)
{
	Elf_Phdr *phdr;
	union headers {
		Elf_Ehdr hdr;
		char buf[PAGE_SIZE];
	} u, *copyu;
	int nbytes;
	int fd;

	/*
	 * Load the main program.
	 */
	if ((fd = open(name, O_RDONLY)) == -1) {
		_rtld_error("Cannot open \"%s\"", "a.out");
		return NULL;
	}
    
	if ((nbytes = read(fd, u.buf, PAGE_SIZE)) == -1) {
		_rtld_error("Read error: %s", strerror(errno));
		return NULL;
	}

	/* Make sure the file is valid */
	if (nbytes < sizeof(Elf_Ehdr)
	    || u.hdr.e_ident[EI_MAG0] != ELFMAG0
	    || u.hdr.e_ident[EI_MAG1] != ELFMAG1
	    || u.hdr.e_ident[EI_MAG2] != ELFMAG2
	    || u.hdr.e_ident[EI_MAG3] != ELFMAG3) {
		_rtld_error("Invalid file format");
		return NULL;
	}
	if (u.hdr.e_ident[EI_CLASS] != ELF_TARG_CLASS
	    || u.hdr.e_ident[EI_DATA] != ELF_TARG_DATA) {
		_rtld_error("Unsupported file layout");
		return NULL;
	}
	if (u.hdr.e_ident[EI_VERSION] != EV_CURRENT
	    || u.hdr.e_version != EV_CURRENT) {
		_rtld_error("Unsupported file version");
		return NULL;
	}
	if (u.hdr.e_type != ET_EXEC && u.hdr.e_type != ET_DYN) {
		_rtld_error("Unsupported file type");
		return NULL;
	}
	if (u.hdr.e_machine != ELF_TARG_MACH) {
		_rtld_error("Unsupported machine");
		return NULL;
	}

	/*
	 * We rely on the program header being in the first page.  This is
	 * not strictly required by the ABI specification, but it seems to
	 * always true in practice.  And, it simplifies things considerably.
	 */
	assert(u.hdr.e_phentsize == sizeof(Elf_Phdr));
	assert(u.hdr.e_phoff + u.hdr.e_phnum*sizeof(Elf_Phdr) <= PAGE_SIZE);
	assert(u.hdr.e_phoff + u.hdr.e_phnum*sizeof(Elf_Phdr) <= nbytes);

	/*
	 * Okay, go ahead and make a copy of it.
	 */
	copyu = (union headers *) xcalloc(sizeof(*copyu));
	*copyu = u;

	*phnum = u.hdr.e_phnum;
	phdr = (Elf_Phdr *) (copyu->buf + copyu->hdr.e_phoff);
	return phdr;
}

void
dump_object(Obj_Entry *pobj)
{
	dbg("mapbase:        %p", pobj->mapbase);
	dbg("mapsize:        %d", pobj->mapsize);
	dbg("textsize:       %d", pobj->textsize);
	dbg("vaddrbase:      %p", pobj->vaddrbase);
	dbg("relocbase:      %p", pobj->relocbase);
	dbg("dynamic:        %p", pobj->dynamic);
	dbg("entry:          %p", pobj->entry);
	dbg("phdr:           %p", pobj->phdr);
	dbg("phsize:         %d", pobj->phsize);
}
