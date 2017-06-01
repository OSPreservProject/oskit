/* $TOG: utils.c /main/128 1997/06/01 13:50:39 sekhar $ */
/*

Copyright (c) 1987  X Consortium

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from the X Consortium.


Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
Copyright 1994 Quarterdeck Office Systems.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Digital and
Quarterdeck not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

DIGITAL AND QUARTERDECK DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT
OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE
OR PERFORMANCE OF THIS SOFTWARE.

*/
/* $XFree86: xc/programs/Xserver/os/utils.c,v 3.27.2.2 1997/07/05 15:55:46 dawes Exp $ */

#include "Xos.h"
#include <stdio.h>
#include <stdlib.h>
#include "misc.h"
#include "X.h"



#ifdef MEMBUG
#define MEM_FAIL_SCALE 100000
long Memory_fail = 0;
#ifdef linux
#include <stdlib.h>  /* for random() */
#endif
#endif


Bool Must_have_memory = FALSE;

/* XALLOC -- X's internal memory allocator.  Why does it return unsigned
 * long * instead of the more common char *?  Well, if you read K&R you'll
 * see they say that alloc must return a pointer "suitable for conversion"
 * to whatever type you really want.  In a full-blown generic allocator
 * there's no way to solve the alignment problems without potentially
 * wasting lots of space.  But we have a more limited problem. We know
 * we're only ever returning pointers to structures which will have to
 * be long word aligned.  So we are making a stronger guarantee.  It might
 * have made sense to make Xalloc return char * to conform with people's
 * expectations of malloc, but this makes lint happier.
 */


unsigned long *
Xalloc (amount)
    unsigned long amount;
{
#if !defined(__STDC__) && !defined(AMOEBA)
    char                *malloc();
#endif
    register pointer  ptr;

    if ((long)amount <= 0) {
        return (unsigned long *)NULL;
    }
    /* aligned extra on long word boundary */
    amount = (amount + (sizeof(long) - 1)) & ~(sizeof(long) - 1);
#ifdef MEMBUG
    if (!Must_have_memory && Memory_fail &&
        ((random() % MEM_FAIL_SCALE) < Memory_fail))
        return (unsigned long *)NULL;
#endif
    if (ptr = (pointer)malloc(amount)) {
        return (unsigned long *)ptr;
    }
    if (Must_have_memory)
        FatalError("Out of memory");
    return (unsigned long *)NULL;
}

/*****************
 * Xcalloc
 *****************/

unsigned long *
Xcalloc (amount)
    unsigned long   amount;
{
    unsigned long   *ret;

    ret = Xalloc (amount);
    if (ret)
        bzero ((char *) ret, (int) amount);
    return ret;
}

/*****************
 * Xrealloc
 *****************/

unsigned long *
Xrealloc (ptr, amount)
    register pointer ptr;
    unsigned long amount;
{
#if !defined(__STDC__) && !defined(AMOEBA)
    char *malloc();
    char *realloc();
#endif

#ifdef MEMBUG
    if (!Must_have_memory && Memory_fail &&
        ((random() % MEM_FAIL_SCALE) < Memory_fail))
        return (unsigned long *)NULL;
#endif
    if ((long)amount <= 0)
    {
        if (ptr && !amount)
            free(ptr);
        return (unsigned long *)NULL;
    }
    amount = (amount + (sizeof(long) - 1)) & ~(sizeof(long) - 1);
    if (ptr)
        ptr = (pointer)realloc((char *)ptr, amount);
    else
        ptr = (pointer)malloc(amount);
    if (ptr)
        return (unsigned long *)ptr;
    if (Must_have_memory)
        FatalError("Out of memory");
    return (unsigned long *)NULL;
}


/*****************
 *  Xfree
 *    calls free
 *****************/

void
Xfree(ptr)
    register pointer ptr;
{
    if (ptr)
        free((char *)ptr);
}
