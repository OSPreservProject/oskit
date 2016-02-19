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
/*
 * Simple polling serial console for the Flux OS toolkit
 */
#ifndef _OSKIT_ARM32_OFW_COM_CONSOLE_H_
#define _OSKIT_ARM32_OFW_COM_CONSOLE_H_

#include <oskit/compiler.h>
#include <oskit/com/stream.h>

struct termios;

OSKIT_BEGIN_DECLS

/*
 * This routine must be called once to initialize the OFW console.
 * If com_params is NULL, a default of 9600,8,N,1 is used.
 */
oskit_error_t
ofw_console_init(struct termios *com_params, oskit_stream_t **out_stream);

/*
 * This is used before things are ready for a real console, like at the
 * beginning of the boot adaptor or multiboot_main. Allows printf very
 * early on.
 */
void
ofw_base_console_init(void);

OSKIT_END_DECLS

#endif /* _OSKIT_ARM32_OFW_COM_CONSOLE_H_ */
