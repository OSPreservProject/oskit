/*
 * Copyright (c) 1996, 1998, 1999 University of Utah and the Flux Group.
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
 * This is provided to allow the use of FreeBSD-style slices, which allow
 * a disklable to be inside a DOS partition.

 * compatabiluty-slice support?
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <oskit/diskpart/diskpart.h>


/*
 * This code is based on the diskpart_lookup_bsd_compat rountine
 *
 * name is a NULL-terminated ASCII string indicating the partition.
 * It is essentially the device name without the drive name part.
 * The FreeBSD slice name `sd0s1e' would just be `s1e'
 *
 * If a slice name is given, it is used.
 * Otherwise, a partition is first assumed to be inside of the
 * (first) BSD slice, if one exists.
 */

struct diskpart *
diskpart_lookup_bsd_string(struct diskpart *array, const char *name)
{

        struct diskpart *partptr;
	const char *nname;
	int num;

	/* null pointer */
        if (name == NULL) {
		return(NULL);
	}

	nname=name;
	partptr=array;

	/*  If no partition then return the whole disk */
	if (nname[0] == 0)
		return(partptr);

	/* is it a slice specification? ('s' followed by int) */
	if ((tolower(nname[0]) == 's') && (isdigit(nname[1]))) {

		nname++; 			/* skip past the s */

		num = atoi(nname); 		/* get the slice number */

		if (num > partptr->nsubs) {
			return(NULL);
		} else {
			if (num>0)
				partptr = &partptr->subs[num-1];
			else
				return(NULL);	/* Slice 0 is no good */
		}
		while (isdigit(nname[0]))
			nname++;

		/* If no partition then return the whole `slice' */
		if (nname[0] == 0)
			return(partptr);

	} else {  /* check first for BSD partition (if any) */

		int i;

		for (i = 0; i < array[0].nsubs; i++)
			if (partptr[0].subs[i].type == DISKPART_BSD) {
				partptr=&partptr->subs[i];
				break;
			}

		/* I could check for VTOC aliasing too ... */
	}


	/* find the partition (in slice, if specified, or aliased to BSD */

	if (isalpha(nname[0])) {
		num = tolower(nname[0]) - 'a';

		if (num > partptr->nsubs) {
			return(NULL);
		} else {
			partptr = &partptr->subs[num];
		}

	} else { 			/* bad name */
		return(NULL);
	}

	return(partptr);
}
