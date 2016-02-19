/*
 * Copyright (c) 1995-1996, 1998 University of Utah and the Flux Group.
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
 * Public definitions for the generic executable interpreter library
 * provided as part of the Flux OS toolkit.
 */
#ifndef	_OSKIT_EXEC_EXEC_H_
#define	_OSKIT_EXEC_EXEC_H_

#include <oskit/compiler.h>
#include <oskit/machine/types.h>

/* XXX */
typedef enum
{
	EXEC_ELF	= 1,
	EXEC_AOUT	= 2,
} exec_format_t;

typedef struct exec_info
{
	/* Format of executable loaded - see above.  */
	exec_format_t format;

	/* Program entrypoint.  */
	oskit_addr_t entry;

	/* Initial data pointer - only some architectures use this.  */
	oskit_addr_t init_dp;

} exec_info_t;

typedef int exec_sectype_t;

/*
 * Flag definitions for section types (exec_sectype_t).
 * Note that the EXEC_SECTYPE_PROT_MASK part of the section type
 * uses the same values as the PROT_* values defined in oskit/c/sys/mman.h,
 * and it may be assumed that this will always be the case.
 * These are also the same values used in Mach 3, and BSD, and Linux, and ...
 */
#define EXEC_SECTYPE_READ		((exec_sectype_t)0x000001)
#define EXEC_SECTYPE_WRITE		((exec_sectype_t)0x000002)
#define EXEC_SECTYPE_EXECUTE		((exec_sectype_t)0x000004)
#define EXEC_SECTYPE_PROT_MASK		((exec_sectype_t)0x000007)
#define EXEC_SECTYPE_ALLOC		((exec_sectype_t)0x000100)
#define EXEC_SECTYPE_LOAD		((exec_sectype_t)0x000200)
#define EXEC_SECTYPE_DEBUG		((exec_sectype_t)0x010000)
#define EXEC_SECTYPE_AOUT_SYMTAB	((exec_sectype_t)0x020000)
#define EXEC_SECTYPE_AOUT_STRTAB	((exec_sectype_t)0x040000)

/* For use with printf's %b format. */
#define EXEC_SECTYPE_FORMAT \
"\20\1READ\2WRITE\3EXEC\11ALLOC\12LOAD\21DEBUG\22AOUT_SYMTAB\23AOUT_STRTAB"

typedef int exec_read_func_t(void *handle, oskit_addr_t file_ofs,
			     void *buf, oskit_size_t size,
			     oskit_size_t *out_actual);

typedef int exec_read_exec_func_t(void *handle,
				  oskit_addr_t file_ofs, oskit_size_t file_size,
			          oskit_addr_t mem_addr, oskit_size_t mem_size,
				  exec_sectype_t section_type);

/*
 * Routines exported from libmach_exec.a
 */

OSKIT_BEGIN_DECLS

/* Generic function to interpret an executable "file"
   and "load" it into "memory".
   Doesn't really know about files, loading, or memory;
   all file I/O and destination memory accesses
   go through provided functions.
   Thus, this is a very generic loading mechanism.

   The read() function is used to read metadata from the file
   into the local address space.

   The read_exec() function is used to load the actual sections.
   It is used for all kinds of sections - code, data, bss, debugging data.
   The 'section_type' parameter specifies what type of section is being loaded.

   For code, data, and bss, the EXEC_SECTYPE_ALLOC flag will be set.
   For code and data (i.e. stuff that's actually loaded from the file),
   EXEC_SECTYPE_LOAD will also be set.
   The EXEC_SECTYPE_PROT_MASK contains the intended access permissions
   for the section.
   'file_size' may be less than 'mem_size';
   the remaining data must be zero-filled.
   'mem_size' is always greater than zero, but 'file_size' may be zero
   (e.g. in the case of a bss section).
   No two read_exec() calls for one executable
   will load data into the same virtual memory page,
   although they may load from arbitrary (possibly overlapping) file positions.

   For sections that aren't normally loaded into the process image
   (e.g. debug sections), EXEC_SECTYPE_ALLOC isn't set,
   but some other appropriate flag is set to indicate the type of section.

   The 'handle' is an opaque pointer which is simply passed on
   to the read() and read_exec() functions.

   On return, the specified info structure is filled in
   with information about the loaded executable.
*/
int exec_load(exec_read_func_t *read, exec_read_exec_func_t *read_exec,
	      void *handle, exec_info_t *out_info);
char *exec_strerror(int err);

/*
 * Error codes
 * XXX these values are a holdover from Mach 3, and probably should be changed.
 */

#define	EX_NOT_EXECUTABLE	6000	/* not a recognized executable format */
#define	EX_WRONG_ARCH		6001	/* valid executable, but wrong arch. */
#define EX_CORRUPT		6002	/* recognized executable, but mangled */
#define EX_BAD_LAYOUT		6003	/* something wrong with the memory or file image layout */

OSKIT_END_DECLS

#endif	/* _OSKIT_EXEC_EXEC_H_ */
