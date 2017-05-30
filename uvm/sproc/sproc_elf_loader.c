/*
 * Copyright (c) 2001 The University of Utah and the Flux Group.
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

#include <oskit/c/unistd.h>
#include <oskit/c/stdio.h>
#include <oskit/c/fcntl.h>	/* O_RDONLY */
#include <oskit/c/strings.h>	/* bzero */
#include <oskit/uvm.h>
#include <oskit/sproc.h>

#include "sproc_internal.h"

struct helper_param {
    struct oskit_sproc		*proc;
    int				fd;
};

static int
read_helper(void *handle, oskit_addr_t file_ofs, void *buf,
	    oskit_size_t size, oskit_size_t *actual)
{
    struct helper_param *param = (struct helper_param*)handle;

    /* XXX: lock or pread()? */
    lseek(param->fd, file_ofs, SEEK_SET);
    *actual = read(param->fd, buf, size);
    if (*actual != size) {
	return -1;
    }
    return 0;
}

static int
read_exec_helper(void *handle, oskit_addr_t file_ofs, oskit_size_t file_size,
		 oskit_addr_t mem_addr, oskit_size_t mem_size,
		 exec_sectype_t section_type)
{
    struct helper_param *param = (struct helper_param*)handle;
    int prot = 0;
    void *mapaddr;

    XPRINTF(OSKIT_DEBUG_LOADER,
	    __FUNCTION__": mem_addr %x filesz %x, memsz %x, type %x\n",
	    mem_addr, file_size, mem_size, section_type);

    if (mem_addr < OSKIT_UVM_MINUSER_ADDRESS) {
	return -1;
    }

    if ((section_type & EXEC_SECTYPE_WRITE)) {
	prot |= PROT_WRITE;
    }
    if ((section_type & EXEC_SECTYPE_READ)) {
	prot |= PROT_READ;
    }
    if ((section_type & EXEC_SECTYPE_EXECUTE)) {
	prot |= PROT_EXEC;
    }
    
    assert(file_size <= mem_size);

    mapaddr = mmap((void*)mem_addr, mem_size, prot,
		   MAP_PRIVATE|MAP_FIXED, param->fd, file_ofs);

    if (mapaddr == 0) {
	return -2;
    }

    /* This happens if we are loading a data section and a bss section
       together. */
    if (file_size < mem_size) {
	bzero((void*)(mem_addr + mem_size), mem_size - file_size);
    }

    return 0;
}

extern int
oskit_sproc_load_elf(struct oskit_sproc *proc, const char *file,
		     exec_info_t *info_out)
{
    int fd;
    struct helper_param param;
    int rc;
    oskit_vmspace_t ovm;

    fd = open(file, O_RDONLY);
    if (fd == -1) {
	return -1;
    }
    param.proc = proc;
    param.fd = fd;
    /* Change the vmspace temporarily (for mmap) */
    ovm = oskit_uvm_vmspace_set(proc->sp_vm);
    /* Load the executable */
    rc = exec_load(read_helper, read_exec_helper, (void*)&param, info_out);
    /* Restore the vmspaec */ 
    oskit_uvm_vmspace_set(ovm);

    close(fd);
    
    return rc;
}


