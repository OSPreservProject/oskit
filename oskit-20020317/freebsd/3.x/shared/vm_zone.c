/*
 * Copyright (c) 1997, 1998 John S. Dyson
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *      notice immediately at the beginning of the file, without modification,
 *      this list of conditions, and the following disclaimer.
 * 2. Absolutely no warranty of function or purpose is made by the author
 *      John S. Dyson.
 *
 * $Id: vm_zone.c,v 1.26 1999/01/10 01:58:29 eivind Exp $
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/sysctl.h>

#include <vm/vm.h>
#include <vm/vm_prot.h>
#include <vm/vm_kern.h>
#include <vm/vm_extern.h>
#include <vm/vm_zone.h>

#include <assert.h>

static MALLOC_DEFINE(M_ZONE, "ZONE", "Zone header");

/*
 * Subroutine same as zinitna, except zone data structure is allocated
 * automatically by malloc.  This routine should normally be used, except
 * in certain tricky startup conditions in the VM system -- then
 * zbootinit and zinitna can be used.  Zinit is the standard zone
 * initialization call.
 */
vm_zone_t
zinit(char *name, int size, int nentries, int flags, int zalloc)
{
        vm_zone_t z;

        z = (vm_zone_t) malloc(sizeof (struct vm_zone), M_ZONE, M_NOWAIT);
        if (z == NULL)
                return NULL;

	/* This is only guaranteed for interrupt level zones */
	if (!(flags & ZONE_INTERRUPT)) {
		panic("%s: only tested for ZONE_INTERRUPT!", __FUNCTION__);
	}

	z->zsize = (size + ZONE_ROUNDING - 1) & ~(ZONE_ROUNDING - 1);
	z->zname = name;
	z->zflags = flags;
	z->zitems = NULL;
	z->zfreecnt = 0;
	z->ztotal = 0;
	z->zfreemin = 1;

        return z;
}


/*
 * Zone allocator/deallocator.  These are interrupt / (or potentially SMP)
 * safe.  The raw zalloc/zfree routines are in the vm_zone header file,
 * and are not interrupt safe, but are fast.
 */
void *
zalloci(vm_zone_t z)
{
        int s;
        void *item;

#ifndef OSKIT
        s = zlock(z);
#endif
        item = _zalloc(z);
#ifndef OSKIT
        zunlock(z, s);
#endif
        return item;
}

void
zfreei(vm_zone_t z, void *item)
{
        int s;

#ifndef OSKIT
        s = zlock(z);
#endif
        _zfree(z, item);
#ifndef OSKIT
        zunlock(z, s);
#endif
        return;
}


/*
 * Internal zone routine.  Not to be called from external (non vm_zone) code.
 */
void *
_zget(vm_zone_t z)
{
	int i;
        int nitems, nbytes;
        void *item;

        if (z == NULL)
                panic("zget: null zone");

	if (!(item = malloc(z->zsize, M_ZONE, M_NOWAIT)))
		panic("zget: can't malloc entry!");
	bzero(item, z->zsize);

	z->ztotal++;
	
	return item;
}
	

