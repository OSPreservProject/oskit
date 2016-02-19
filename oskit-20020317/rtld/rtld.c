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
 *      $\Id: rtld.c,v 1.1 1999/02/19 15:26:18 stoller Exp stoller $
 */

/*
 * Dynamic linker for ELF.
 *
 * John Polstra <jdp@polstra.com>.
 */

#ifndef __GNUC__
#error "GCC is needed to compile this file"
#endif

#include <sys/param.h>
#include <sys/mman.h>

#include <dlfcn.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>

#include "debug.h"
#include "rtld.h"

#define END_SYM		"end"

/* Types. */
typedef void (*func_ptr_type)();

/*
 * Function declarations.
 */
static void call_fini_functions(Obj_Entry *);
static void call_init_functions(Obj_Entry *);
static void die(void);
static void digest_dynamic(Obj_Entry *);
static Obj_Entry *digest_phdr(const Elf_Phdr *, int, caddr_t);
static Obj_Entry *dlcheck(void *);
static char *find_library(const char *, const Obj_Entry *);
static const char *gethints(void);
static int load_needed_objects(Obj_Entry *);
static Obj_Entry *load_object(char *);
static Obj_Entry *obj_from_addr(const void *);
static int relocate_objects(Obj_Entry *, bool);
static void rtld_exit(void);
static char *search_library_path(const char *, const char *);
static void unref_object_dag(Obj_Entry *);

void xprintf(const char *, ...);

#ifdef RTLDDEBUG
static const char *basename(const char *);
#endif

/*
 * Uh, not exactly an oskit thing.
 */
#define mprotect(a, b, c)	(0)
#define munmap(a, b)		sfree(a, b)

/* Assembly language entry point for lazy binding. */
extern void _rtld_bind_start(void);

/*
 * Data declarations.
 */
static char *error_message;	/* Message for dlerror(), or NULL */
static bool trust;		/* False for setuid and setgid programs */

static char *ld_library_path;	/* Environment variable for search path */
static char *ld_tracing;	/* Called from ldd to print libs */
static Obj_Entry **main_tail;	/* Value of obj_tail after loading main and
				   its needed shared libraries */
static Obj_Entry *obj_list;	/* Head of linked list of shared objects */
static Obj_Entry **obj_tail;	/* Link field of last object in list */
static Obj_Entry *obj_main;	/* The main program shared object */

extern Elf_Dyn _DYNAMIC;

/*
 * Boot the dynamic loader. This entrypoint would be used to initialize
 * the rtld in a newly booted oskit kernel. The caller passes in the name
 * of the a.out file from which to grab the dynamic load information.
 */
int
oskit_boot_rtld(char *filename)
{
	int phnum;
	caddr_t entry;
	Elf_Phdr *phdr;
	static int done;

	if (done)
		return 0;

	done  = 1;
	trust = 1;
	rtld_debug = 1;
	
	if ((ld_library_path = getenv("LD_LIBRARY_PATH")) == NULL)
		ld_library_path = ".";
	dbg("oskit_boot_rtld: LD_LIBRARY_PATH = %s\n", ld_library_path);
	
	/* Make the object list empty */
	obj_list = NULL;
	obj_tail = &obj_list;

	if (&_DYNAMIC != 0) {
		obj_main = CNEW(Obj_Entry);

		if (obj_main == NULL)
			die();
		
		obj_main->dynamic   = (const Elf_Dyn *) &_DYNAMIC;
		obj_main->relocbase = 0;
		obj_main->path      = "Oskit Kernel";
	}
	else if (filename &&
		 (phdr = load_headers(filename, &phnum, &entry)) != NULL) {

		obj_main = digest_phdr(phdr, phnum, entry);
		if (obj_main == NULL)
			die();

		obj_main->path = xstrdup(filename);
	}
	else {
		/*
		 * This is okay. Won't be able to link against main
		 * program symbols, which will eventually result in
		 * failure. 
		 */
		return 0;
	}
	
	obj_main->mainprog = true;
	digest_dynamic(obj_main);

	/* Link the main program into the list of objects. */
	*obj_tail = obj_main;
	obj_tail = &obj_main->next;
	obj_main->refcount++;

	return 0;
}

/*
 * Initialize the dynamic loader. This entrypoint is used when intializing
 * the library as part of a shared object. The kernel has loaded and linked
 * the new shared object, but in order to initiate subsequent loads, we need
 * to have the object list. We could recreate it, but for now the kernel
 * just passes in the object list. This is rather bogus, since we do not
 * want to share the object list with the kernel, cause it might get modified
 * here. But, lets do this for now and see where it takes us.
 *
 * How is this supposed to work? Well, when a dynamic link program is loaded
 * by the FreeBSD kernel, it really loads a PIC version of the rtld library,
 * that has been linked with a PIC version of the C library. This library
 * relocates itself, and then loads/relocates the program that is really being
 * run. We could do something like that too, but it sounds like overkill
 * unless we really want to play with using different memory allocators, in
 * which case the object list cannot be shared with the oskit kernel.
 */
int
oskit_init_rtld(Obj_Entry *obj)
{
	static int done;

	if (done)
		return 0;

	done  = 1;
	trust = 1;
	rtld_debug = 1;
	
	if ((ld_library_path = getenv("LD_LIBRARY_PATH")) == NULL)
		ld_library_path = ".";
	dbg("oskit_init_rtld: LD_LIBRARY_PATH = %s\n", ld_library_path);

	if (obj) {
		/*
		 * Scan down the list to find the last object.
		 */
		obj_list = obj;
		while (obj->next)
			obj = obj->next;
		obj_tail = &obj->next;
	}
	else {
		/* Make the object list empty */
		obj_list = NULL;
		obj_tail = &obj_list;
	}
	return 0;
}

caddr_t
_rtld_bind(const Obj_Entry *obj, Elf_Word reloff)
{
    const Elf_Rel *rel;
    const Elf_Sym *def;
    const Obj_Entry *defobj;
    Elf_Addr *where;
    caddr_t target;

    if (obj->pltrel)
	rel = (const Elf_Rel *) ((caddr_t) obj->pltrel + reloff);
    else
	rel = (const Elf_Rel *) ((caddr_t) obj->pltrela + reloff);

    where = (Elf_Addr *) (obj->relocbase + rel->r_offset);
    def = find_symdef(ELF_R_SYM(rel->r_info), obj, &defobj, true);
    if (def == NULL)
	die();

    target = (caddr_t) (defobj->relocbase + def->st_value);

    dbg("\"%s\" in \"%s\" ==> %p in \"%s\"",
      defobj->strtab + def->st_name, basename(obj->path),
      target, basename(defobj->path));

    *where = (Elf_Addr) target;
    return target;
}

/*
 * Error reporting function.  Use it like printf.  If formats the message
 * into a buffer, and sets things up so that the next call to dlerror()
 * will return the message.
 */
void
_rtld_error(const char *fmt, ...)
{
    static char buf[512];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    error_message = buf;
    va_end(ap);
}

#ifdef RTLDDEBUG
static const char *
basename(const char *name)
{
    const char *p = strrchr(name, '/');
    return p != NULL ? p + 1 : name;
}
#endif

static void
call_fini_functions(Obj_Entry *first)
{
    Obj_Entry *obj;

    for (obj = first;  obj != NULL;  obj = obj->next)
	if (obj->fini != NULL)
	    (*obj->fini)();
}

static void
call_init_functions(Obj_Entry *first)
{
    if (first != NULL) {
	call_init_functions(first->next);
	if (first->init != NULL)
	    (*first->init)();
    }
}

static void
die(void)
{
    const char *msg = dlerror();

    if (msg == NULL)
	msg = "Fatal error";
    errx(1, "%s", msg);
}

/*
 * Process a shared object's DYNAMIC section, and save the important
 * information in its Obj_Entry structure.
 */
static void
digest_dynamic(Obj_Entry *obj)
{
    const Elf_Dyn *dynp;
    Needed_Entry **needed_tail = &obj->needed;
    const Elf_Dyn *dyn_rpath = NULL;
    int plttype = DT_REL;

    for (dynp = obj->dynamic;  dynp->d_tag != DT_NULL;  dynp++) {
	switch (dynp->d_tag) {

	case DT_REL:
	    obj->rel = (const Elf_Rel *) (obj->relocbase + dynp->d_un.d_ptr);
	    break;

	case DT_RELSZ:
	    obj->relsize = dynp->d_un.d_val;
	    break;

	case DT_RELENT:
	    assert(dynp->d_un.d_val == sizeof(Elf_Rel));
	    break;

	case DT_JMPREL:
	    obj->pltrel = (const Elf_Rel *)
	      (obj->relocbase + dynp->d_un.d_ptr);
	    break;

	case DT_PLTRELSZ:
	    obj->pltrelsize = dynp->d_un.d_val;
	    break;

	case DT_RELA:
	    obj->rela = (const Elf_Rela *) (obj->relocbase + dynp->d_un.d_ptr);
	    break;

	case DT_RELASZ:
	    obj->relasize = dynp->d_un.d_val;
	    break;

	case DT_RELAENT:
	    assert(dynp->d_un.d_val == sizeof(Elf_Rela));
	    break;

	case DT_PLTREL:
	    plttype = dynp->d_un.d_val;
	    assert(dynp->d_un.d_val == DT_REL || plttype == DT_RELA);
	    break;

	case DT_SYMTAB:
	    obj->symtab = (const Elf_Sym *)
	      (obj->relocbase + dynp->d_un.d_ptr);
	    break;

	case DT_SYMENT:
	    assert(dynp->d_un.d_val == sizeof(Elf_Sym));
	    break;

	case DT_STRTAB:
	    obj->strtab = (const char *) (obj->relocbase + dynp->d_un.d_ptr);
	    break;

	case DT_STRSZ:
	    obj->strsize = dynp->d_un.d_val;
	    break;

	case DT_HASH:
	    {
		const Elf_Addr *hashtab = (const Elf_Addr *)
		  (obj->relocbase + dynp->d_un.d_ptr);
		obj->nbuckets = hashtab[0];
		obj->nchains = hashtab[1];
		obj->buckets = hashtab + 2;
		obj->chains = obj->buckets + obj->nbuckets;
	    }
	    break;

	case DT_NEEDED:
	    assert(!obj->rtld);
	    {
		Needed_Entry *nep = NEW(Needed_Entry);
		nep->name = dynp->d_un.d_val;
		nep->obj = NULL;
		nep->next = NULL;

		*needed_tail = nep;
		needed_tail = &nep->next;
	    }
	    break;

	case DT_PLTGOT:
	    obj->got = (Elf_Addr *) (obj->relocbase + dynp->d_un.d_ptr);
	    break;

	case DT_TEXTREL:
	    obj->textrel = true;
	    break;

	case DT_SYMBOLIC:
	    obj->symbolic = true;
	    break;

	case DT_RPATH:
	    /*
	     * We have to wait until later to process this, because we
	     * might not have gotten the address of the string table yet.
	     */
	    dyn_rpath = dynp;
	    break;

	case DT_SONAME:
	    /* Not used by the dynamic linker. */
	    break;

	case DT_INIT:
	    obj->init = (void (*)(void)) (obj->relocbase + dynp->d_un.d_ptr);
	    break;

	case DT_FINI:
	    obj->fini = (void (*)(void)) (obj->relocbase + dynp->d_un.d_ptr);
	    break;

	case DT_DEBUG:
	    /* XXX - not implemented yet */
	    dbg("DT_DEBUG not supported!");
	    break;

	default:
	    xprintf("Ignored d_tag %d\n",dynp->d_tag);
            break;
	}
    }

    obj->traced = false;

    if (plttype == DT_RELA) {
	obj->pltrela = (const Elf_Rela *) obj->pltrel;
	obj->pltrel = NULL;
	obj->pltrelasize = obj->pltrelsize;
	obj->pltrelsize = 0;
    }

    if (dyn_rpath != NULL)
	obj->rpath = obj->strtab + dyn_rpath->d_un.d_val;
}

/*
 * Process a shared object's program header.  This is used only for the
 * main program, when the kernel has already loaded the main program
 * into memory before calling the dynamic linker.  It creates and
 * returns an Obj_Entry structure.
 */
static Obj_Entry *
digest_phdr(const Elf_Phdr *phdr, int phnum, caddr_t entry)
{
    Obj_Entry *obj = CNEW(Obj_Entry);
    const Elf_Phdr *phlimit = phdr + phnum;
    const Elf_Phdr *ph;
    int nsegs = 0;

    for (ph = phdr;  ph < phlimit;  ph++) {
	switch (ph->p_type) {

	case PT_PHDR:
	    /*assert((const Elf_Phdr *) ph->p_vaddr == phdr);*/
	    obj->phdr = (const Elf_Phdr *) ph->p_vaddr;
	    obj->phsize = ph->p_memsz;
	    break;

	case PT_LOAD:
	    assert(nsegs < 2);
	    if (nsegs == 0) {	/* First load segment */
		obj->vaddrbase = trunc_page(ph->p_vaddr);
		obj->mapbase = (caddr_t) obj->vaddrbase;
		obj->relocbase = obj->mapbase - obj->vaddrbase;
		obj->textsize = round_page(ph->p_vaddr + ph->p_memsz) -
		  obj->vaddrbase;
	    } else {		/* Last load segment */
		obj->mapsize = round_page(ph->p_vaddr + ph->p_memsz) -
		  obj->vaddrbase;
	    }
	    nsegs++;
	    break;

	case PT_DYNAMIC:
	    obj->dynamic = (const Elf_Dyn *) ph->p_vaddr;
	    break;
	}
    }
    assert(nsegs == 2);

    obj->entry = entry;
    return obj;
}

static Obj_Entry *
dlcheck(void *handle)
{
    Obj_Entry *obj;

    for (obj = obj_list;  obj != NULL;  obj = obj->next)
	if (obj == (Obj_Entry *) handle)
	    break;

    if (obj == NULL || obj->dl_refcount == 0) {
	_rtld_error("Invalid shared object handle %p", handle);
	return NULL;
    }
    return obj;
}

/*
 * Hash function for symbol table lookup.  Don't even think about changing
 * this.  It is specified by the System V ABI.
 */
unsigned long
elf_hash(const char *name)
{
    const unsigned char *p = (const unsigned char *) name;
    unsigned long h = 0;
    unsigned long g;

    while (*p != '\0') {
	h = (h << 4) + *p++;
	if ((g = h & 0xf0000000) != 0)
	    h ^= g >> 24;
	h &= ~g;
    }
    return h;
}

/*
 * Find the library with the given name, and return its full pathname.
 * The returned string is dynamically allocated.  Generates an error
 * message and returns NULL if the library cannot be found.
 *
 * If the second argument is non-NULL, then it refers to an already-
 * loaded shared object, whose library search path will be searched.
 *
 * The search order is:
 *   LD_LIBRARY_PATH
 *   ldconfig hints
 *   rpath in the referencing file
 *   /usr/lib
 */
static char *
find_library(const char *name, const Obj_Entry *refobj)
{
    char *pathname;

    if (strchr(name, '/') != NULL) {	/* Hard coded pathname */
	if (name[0] != '/' && !trust) {
	    _rtld_error("Absolute pathname required for shared object \"%s\"",
	      name);
	    return NULL;
	}
	return xstrdup(name);
    }

    dbg(" Searching for \"%s\"", name);

    if ((pathname = search_library_path(name, ld_library_path)) != NULL ||
      (pathname = search_library_path(name, gethints())) != NULL ||
      (refobj != NULL &&
      (pathname = search_library_path(name, refobj->rpath)) != NULL) ||
      (pathname = search_library_path(name, STANDARD_LIBRARY_PATH)) != NULL)
	return pathname;

    _rtld_error("Shared object \"%s\" not found", name);
    return NULL;
}

/*
 * Given a symbol number in a referencing object, find the corresponding
 * definition of the symbol.  Returns a pointer to the symbol, or NULL if
 * no definition was found.  Returns a pointer to the Obj_Entry of the
 * defining object via the reference parameter DEFOBJ_OUT.
 */
const Elf_Sym *
find_symdef(unsigned long symnum, const Obj_Entry *refobj,
    const Obj_Entry **defobj_out, bool in_plt)
{
    const Elf_Sym *ref;
    const Elf_Sym *strongdef;
    const Elf_Sym *weakdef;
    const Obj_Entry *obj;
    const Obj_Entry *strongobj;
    const Obj_Entry *weakobj;
    const char *name;
    unsigned long hash;

    ref = refobj->symtab + symnum;
    name = refobj->strtab + ref->st_name;
    hash = elf_hash(name);

    if (refobj->symbolic) {	/* Look first in the referencing object */
	const Elf_Sym *def = symlook_obj(name, hash, refobj, in_plt);
	if (def != NULL) {
	    *defobj_out = refobj;
	    return def;
	}
    }

    /*
     * Look in all loaded objects.  Skip the referencing object, if
     * we have already searched it.  We keep track of the first weak
     * definition and the first strong definition we encounter.  If
     * we find a strong definition we stop searching, because there
     * won't be anything better than that.
     */
    strongdef = weakdef = NULL;
    strongobj = weakobj = NULL;
    for (obj = obj_list;  obj != NULL;  obj = obj->next) {
	if (obj != refobj || !refobj->symbolic) {
	    const Elf_Sym *def = symlook_obj(name, hash, obj, in_plt);
	    if (def != NULL) {
		if (ELF_ST_BIND(def->st_info) == STB_WEAK) {
		    if (weakdef == NULL) {
			weakdef = def;
			weakobj = obj;
		    }
		} else {
		    strongdef = def;
		    strongobj = obj;
		    break;	/* We are done. */
		}
	    }
	}
    }

    if (strongdef != NULL) {
	*defobj_out = strongobj;
	return strongdef;
    }
    if (weakdef != NULL) {
	*defobj_out = weakobj;
	return weakdef;
    }
    _rtld_error("%s: Undefined symbol \"%s\"", refobj->path, name);
    return NULL;
}

/*
 * Return the search path from the ldconfig hints file, reading it if
 * necessary.  Returns NULL if there are problems with the hints file,
 * or if the search path there is empty.
 */
static const char *
gethints(void)
{
    static char *hints;

    if (hints == NULL) {
	int fd;
	struct elfhints_hdr hdr;
	char *p;

	/* Keep from trying again in case the hints file is bad. */
	hints = "";

	if ((fd = open(_PATH_ELF_HINTS, O_RDONLY)) == -1)
	    return NULL;
	if (read(fd, &hdr, sizeof hdr) != sizeof hdr ||
	  hdr.magic != ELFHINTS_MAGIC ||
	  hdr.version != 1) {
	    close(fd);
	    return NULL;
	}
	p = xmalloc(hdr.dirlistlen + 1);
	if (lseek(fd, hdr.strtab + hdr.dirlist, SEEK_SET) == -1 ||
	  read(fd, p, hdr.dirlistlen + 1) != hdr.dirlistlen + 1) {
	    free(p);
	    close(fd);
	    return NULL;
	}
	hints = p;
	close(fd);
    }
    return hints[0] != '\0' ? hints : NULL;
}

/*
 * Given a shared object, traverse its list of needed objects, and load
 * each of them.  Returns 0 on success.  Generates an error message and
 * returns -1 on failure.
 */
static int
load_needed_objects(Obj_Entry *first)
{
    Obj_Entry *obj;

    for (obj = first;  obj != NULL;  obj = obj->next) {
	Needed_Entry *needed;

	for (needed = obj->needed;  needed != NULL;  needed = needed->next) {
	    const char *name = obj->strtab + needed->name;
	    char *path = find_library(name, obj);

	    needed->obj = NULL;
	    if (path == NULL && !ld_tracing)
		return -1;

	    if (path) {
		needed->obj = load_object(path);
		if (needed->obj == NULL && !ld_tracing)
		    return -1;		/* XXX - cleanup */
	    }
	}
    }

    return 0;
}

/*
 * Load a shared object into memory, if it is not already loaded.  The
 * argument must be a string allocated on the heap.  This function assumes
 * responsibility for freeing it when necessary.
 *
 * Returns a pointer to the Obj_Entry for the object.  Returns NULL
 * on failure.
 */
static Obj_Entry *
load_object(char *path)
{
    Obj_Entry *obj;

    for (obj = obj_list->next;  obj != NULL;  obj = obj->next)
	if (strcmp(obj->path, path) == 0)
	    break;

    if (obj == NULL) {	/* First use of this object, so we must map it in */
	int fd;

	if ((fd = open(path, O_RDONLY)) == -1) {
	    _rtld_error("Cannot open \"%s\"", path);
	    return NULL;
	}
	obj = read_object(fd);
	close(fd);
	if (obj == NULL) {
	    free(path);
	    return NULL;
	}

	obj->path = path;
	digest_dynamic(obj);

	*obj_tail = obj;
	obj_tail = &obj->next;

	dbg("  %p .. %p: %s", obj->mapbase,
	  obj->mapbase + obj->mapsize - 1, obj->path);
	if (obj->textrel)
	    dbg("  WARNING: %s has impure text", obj->path);
    } else
	free(path);

    obj->refcount++;
    return obj;
}

static Obj_Entry *
obj_from_addr(const void *addr)
{
    unsigned long endhash;
    Obj_Entry *obj;

    endhash = elf_hash(END_SYM);
    for (obj = obj_list;  obj != NULL;  obj = obj->next) {
	const Elf_Sym *endsym;

	if (addr < (void *) obj->mapbase)
	    continue;
	if ((endsym = symlook_obj(END_SYM, endhash, obj, true)) == NULL)
	    continue;	/* No "end" symbol?! */
	if (addr < (void *) (obj->relocbase + endsym->st_value))
	    return obj;
    }
    return NULL;
}

/*
 * Relocate newly-loaded shared objects.  The argument is a pointer to
 * the Obj_Entry for the first such object.  All objects from the first
 * to the end of the list of objects are relocated.  Returns 0 on success,
 * or -1 on failure.
 */
static int
relocate_objects(Obj_Entry *first, bool bind_now)
{
    Obj_Entry *obj;

    for (obj = first;  obj != NULL;  obj = obj->next) {
	dbg("relocating \"%s\"", obj->path);
	if (obj->nbuckets == 0 || obj->nchains == 0 || obj->buckets == NULL ||
	    obj->symtab == NULL || obj->strtab == NULL) {
	    _rtld_error("%s: Shared object has no run-time symbol table",
	      obj->path);
	    return -1;
	}

	if (obj->textrel) {
	    /* There are relocations to the write-protected text segment. */
	    if (mprotect(obj->mapbase, obj->textsize,
	      PROT_READ|PROT_WRITE|PROT_EXEC) == -1) {
		_rtld_error("%s: Cannot write-enable text segment: %s",
		  obj->path, strerror(errno));
		return -1;
	    }
	}

	/* Process the non-PLT relocations. */
	if (reloc_non_plt(obj))
		return -1;

	if (obj->textrel) {	/* Re-protected the text segment. */
	    if (mprotect(obj->mapbase, obj->textsize,
	      PROT_READ|PROT_EXEC) == -1) {
		_rtld_error("%s: Cannot write-protect text segment: %s",
		  obj->path, strerror(errno));
		return -1;
	    }
	}

	/* Process the PLT relocations. */
	if (reloc_plt(obj, bind_now))
		return -1;

	/*
	 * Set up the magic number and version in the Obj_Entry.  These
	 * were checked in the crt1.o from the original ElfKit, so we
	 * set them for backward compatibility.
	 */
	obj->magic = RTLD_MAGIC;
	obj->version = RTLD_VERSION;

	/* Set the special GOT entries. */
	if (obj->got) {
#ifdef __i386__
	    obj->got[1] = (Elf_Addr) obj;
	    obj->got[2] = (Elf_Addr) &_rtld_bind_start;
#endif
#ifdef __alpha__
	    /* This function will be called to perform the relocation.  */
	    obj->got[2] = (Elf_Addr) &_rtld_bind_start;
	    /* Identify this shared object */
	    obj->got[3] = (Elf_Addr) obj;
#endif
	}
    }

    return 0;
}

/*
 * Cleanup procedure.  It will be called (by the atexit mechanism) just
 * before the process exits.
 */
static void
rtld_exit(void)
{
    dbg("rtld_exit()");
    call_fini_functions(obj_list->next);
}

static char *
search_library_path(const char *name, const char *path)
{
    size_t namelen = strlen(name);
    const char *p = path;

    if (p == NULL)
	return NULL;

    p += strspn(p, ":;");
    while (*p != '\0') {
	size_t len = strcspn(p, ":;");

	if (*p == '/' || trust) {
	    char *pathname;
	    const char *dir = p;
	    size_t dirlen = len;

	    pathname = xmalloc(dirlen + 1 + namelen + 1);
	    strncpy(pathname, dir, dirlen);
	    pathname[dirlen] = '/';
	    strcpy(pathname + dirlen + 1, name);

	    dbg("  Trying \"%s\"", pathname);
	    if (access(pathname, F_OK) == 0)		/* We found it */
		return pathname;

	    free(pathname);
	}
	p += len;
	p += strspn(p, ":;");
    }

    return NULL;
}

int
dlclose(void *handle)
{
    Obj_Entry *root = dlcheck(handle);

    if (root == NULL)
	return -1;

    root->dl_refcount--;
    unref_object_dag(root);
    if (root->refcount == 0) {	/* We are finished with some objects. */
	Obj_Entry *obj;
	Obj_Entry **linkp;

	/* Finalize objects that are about to be unmapped. */
	for (obj = obj_list->next;  obj != NULL;  obj = obj->next)
	    if (obj->refcount == 0 && obj->fini != NULL)
		(*obj->fini)();

	/* Unmap all objects that are no longer referenced. */
	linkp = &obj_list->next;
	while ((obj = *linkp) != NULL) {
	    if (obj->refcount == 0) {
		munmap(obj->mapbase, obj->mapsize);
		free(obj->path);
		while (obj->needed != NULL) {
		    Needed_Entry *needed = obj->needed;
		    obj->needed = needed->next;
		    free(needed);
		}
		*linkp = obj->next;
		free(obj);
	    } else
		linkp = &obj->next;
	}
	obj_tail = linkp;
    }

    return 0;
}

const char *
dlerror(void)
{
    char *msg = error_message;
    error_message = NULL;
    return msg;
}

void *
dlopen(const char *name, int mode)
{
    Obj_Entry **old_obj_tail = obj_tail;
    Obj_Entry *obj = NULL;

    mode = RTLD_LAZY;

    if (name == NULL)
	obj = obj_main;
    else {
	char *path = find_library(name, NULL);
	if (path != NULL)
	    obj = load_object(path);
    }

    if (obj) {
	obj->dl_refcount++;
	if (*old_obj_tail != NULL) {		/* We loaded something new. */
	    assert(*old_obj_tail == obj);

	    /* XXX - Clean up properly after an error. */
	    if (load_needed_objects(obj) == -1) {
		obj->dl_refcount--;
		obj = NULL;
	    } else if (relocate_objects(obj, mode == RTLD_NOW) == -1) {
		obj->dl_refcount--;
		obj = NULL;
	    } else
		call_init_functions(obj);
	}
    }

    return obj;
}

void *
dlsym(void *handle, const char *name)
{
    const Obj_Entry *obj;
    unsigned long hash;
    const Elf_Sym *def;

    hash = elf_hash(name);
    def = NULL;

    if (handle == NULL || handle == RTLD_NEXT) {
	void *retaddr;

	retaddr = __builtin_return_address(0);	/* __GNUC__ only */
	if ((obj = obj_from_addr(retaddr)) == NULL) {
	    _rtld_error("Cannot determine caller's shared object");
	    return NULL;
	}
	if (handle == NULL)	/* Just the caller's shared object. */
	    def = symlook_obj(name, hash, obj, true);
	else {			/* All the shared objects after the caller's */
	    while ((obj = obj->next) != NULL)
		if ((def = symlook_obj(name, hash, obj, true)) != NULL)
		    break;
	}
    } else {
	if ((obj = dlcheck(handle)) == NULL)
	    return NULL;

	if (obj->mainprog) {
	    /* Search main program and all libraries loaded by it. */
	    for ( ;  obj != *main_tail;  obj = obj->next)
		if ((def = symlook_obj(name, hash, obj, true)) != NULL)
		    break;
	} else {
	    /*
	     * XXX - This isn't correct.  The search should include the whole
	     * DAG rooted at the given object.
	     */
	    def = symlook_obj(name, hash, obj, true);
	}
    }

    if (def != NULL)
	return obj->relocbase + def->st_value;

    _rtld_error("Undefined symbol \"%s\"", name);
    return NULL;
}

/*
 * This entrypoint is intended to load a self contained object file into
 * its own space. That is, it is *not* linked against the current object
 * file, but starts out new. To accomplish this, I just rebind the obj
 * list to null, load the specified file, and then restore the list of
 * objects. The new object list is returned to the caller so that it can
 * "unload" the object when it exits. Other values are also returned:
 * 1) The location of the requested entrypoint. 2) A pointer to the program
 * header, which is passed to the new program so it can do its own rtld
 * initialization (the program header is part of the text region).
 *
 * XXX: Locking ...
 */
void *
dlload(const char *filename,
       const char *entryname, void **entrypoint, void **phdr)
{
    Obj_Entry	**old_obj_tail, *old_obj_list;
    Obj_Entry	*obj = NULL;
    const Elf_Sym *def;
    int		fd;
    char	*path;

    if ((path = find_library(filename, NULL)) == NULL) {
	    _rtld_error("Cannot open \"%s\"", filename);
	    return NULL;
    }
	
    /* Save off the old object list. */
    old_obj_list = obj_list;
    old_obj_tail = obj_tail;

    /* Make the object list empty for the duration of this load. */
    obj_list = NULL;
    obj_tail = &obj_list;

    /*
     * open the file, and read it in.
     */
    if ((fd = open(path, O_RDONLY)) == -1) {
	    _rtld_error("Cannot open \"%s\"", path);
	    goto bad;
    }
    obj = read_object(fd);
    close(fd);
    if (obj == NULL)
	    goto bad;

    obj->refcount++;
    obj->path = path;
    *obj_tail = obj;
    obj_tail = &obj->next;
    
    digest_dynamic(obj);

    dbg("  %p .. %p: %s", obj->mapbase,
	obj->mapbase + obj->mapsize - 1, obj->path);

    /*
     * Load any needed objects.
     */
    if (load_needed_objects(obj) == -1) {
	    dbg("dlload: %s - Could not load needed objects\n", filename);
	    goto bad;
    }

    /*
     * Relocate newly loaded object.
     */
    if (relocate_objects(obj, false) == -1) {
	    dbg("dlload: %s - Could not relocate\n", filename);
	    goto bad;
    }

    /*
     * Find the desired entrypoint.
     */
    if ((def = symlook_obj(entryname, elf_hash(entryname), obj, true))
	== NULL) {
	    dbg("dlload: %s - Could not find entrypoint %s\n",
		filename, entryname);
	    goto bad;
    }
    obj->dl_refcount++;
    *entrypoint = obj->relocbase + def->st_value;
    *phdr = obj->mapbase;

    /* Restore the old object list */
    obj_list = old_obj_list;
    obj_tail = old_obj_tail;

    return obj;

 bad:
    /*
     * Free up all the object information.
     */
    for (obj = obj_list; obj != NULL; ) {
	    Obj_Entry	*nextobj = obj->next;

	    sfree(obj->mapbase, obj->mapsize);
	    free(obj->path);
	    free(obj);
	    obj = nextobj;
    }

    /* Restore the old object list */
    obj_list = old_obj_list;
    obj_tail = old_obj_tail;

    return 0;
}

/*
 * Unload a shared object (and any other related shared objects).
 */
int
dlunload(void *handle)
{
    Obj_Entry	*obj = handle;

    assert(obj);

    obj->dl_refcount--;
    unref_object_dag(obj);

    /*
     * Free up all the object information.
     */
    for (; obj != NULL; ) {
	    Obj_Entry	*nextobj = obj->next;
	    
	    assert(obj->refcount == 0);
	    sfree(obj->mapbase, obj->mapsize);
	    free(obj->path);
	    free(obj);
	    obj = nextobj;
    }
    return 0;
}

/*
 * Search the symbol table of a single shared object for a symbol of
 * the given name.  Returns a pointer to the symbol, or NULL if no
 * definition was found.
 *
 * The symbol's hash value is passed in for efficiency reasons; that
 * eliminates many recomputations of the hash value.
 */
const Elf_Sym *
symlook_obj(const char *name, unsigned long hash, const Obj_Entry *obj,
  bool in_plt)
{
    unsigned long symnum = obj->buckets[hash % obj->nbuckets];

    while (symnum != STN_UNDEF) {
	const Elf_Sym *symp;
	const char *strp;

	assert(symnum < obj->nchains);
	symp = obj->symtab + symnum;
	assert(symp->st_name != 0);
	strp = obj->strtab + symp->st_name;

	if (strcmp(name, strp) == 0)
	    return symp->st_shndx != SHN_UNDEF ||
	      (!in_plt && symp->st_value != 0 &&
	      ELF_ST_TYPE(symp->st_info) == STT_FUNC) ? symp : NULL;

	symnum = obj->chains[symnum];
    }

    return NULL;
}

static void
unref_object_dag(Obj_Entry *root)
{
    assert(root->refcount != 0);
    root->refcount--;
    if (root->refcount == 0) {
	const Needed_Entry *needed;

	for (needed = root->needed;  needed != NULL;  needed = needed->next)
	    unref_object_dag(needed->obj);
    }
}

/*
 * Non-mallocing printf, for use by malloc itself.
 * XXX - This doesn't belong in this module.
 */
void
xprintf(const char *fmt, ...)
{
    char buf[256];
    va_list ap;

    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    (void)write(1, buf, strlen(buf));
    va_end(ap);
}
