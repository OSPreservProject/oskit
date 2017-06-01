/*
 * Copyright (c) 1998 The University of Utah and the Flux Group.
 * All rights reserved.
 * 
 * This file is part of the OSKit Linux Glue Libraries, which are free
 * software, also known as "open source;" you can redistribute them and/or
 * modify them under the terms of the GNU General Public License (GPL),
 * version 2, as published by the Free Software Foundation (FSF).
 * 
 * The OSKit is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GPL for more details.  You should have
 * received a copy of the GPL along with the OSKit; see the file COPYING.  If
 * not, write to the FSF, 59 Temple Place #330, Boston, MA 02111-1307, USA.
 */
/*
 * Define the oskit_pfq_root variable,
 * which HPFQ users need to set to the root of their scheduling tree.
 */

#ifdef HPFQ
#include <oskit/hpfq.h>
#include <oskit/c/stddef.h>

pfq_sched_t *oskit_pfq_root = NULL;

#endif /* HPFQ */
