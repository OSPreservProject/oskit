/*
 * Copyright (c) 1997-1998 University of Utah and the Flux Group.
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

#ifndef _OSKIT_IO_MOUSE_
#define _OSKIT_IO_MOUSE_

#include <oskit/com.h>
#include <oskit/types.h>
#include <oskit/io/ttystream.h>

/*
 * XXX THIS INTERFACE WILL CHANGE!  DO NOT RELY ON IT!
 */


#define P_MS            0                       /* Microsoft */
#define P_MSC           1                       /* Mouse Systems Corp */
#define P_MM            2                       /* MMseries */
#define P_LOGI          3                       /* Logitech */
#define P_BM            4                       /* BusMouse ??? */
#define P_LOGIMAN       5                       /* MouseMan / TrackMan
                                                   [CHRIS-211092] */
#define P_PS2           6                       /* PS/2 mouse */
#define P_MMHIT         7                       /* MM_HitTab */
#define P_GLIDEPOINT    8                       /* ALPS GlidePoint */
#define P_MSINTELLIMOUSE  9                     /* Microsoft IntelliMouse */


typedef struct {
	int 	dx, dy;
	int	buttons;
} oskit_mouse_packet_t;


int oskit_mouse_init(oskit_ttystream_t *ttystrm, int type, int rate);

oskit_mouse_packet_t *oskit_mouse_get_packet(void);


#endif /* _OSKIT_IO_MOUSE_ */

