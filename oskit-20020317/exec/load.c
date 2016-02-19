/*
 * Copyright (c) 1995, 1998 University of Utah and the Flux Group.
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

#include "exec.h"

int exec_load(exec_read_func_t *read, exec_read_exec_func_t *read_exec,
	      void *handle, exec_info_t *out_info)
{
	int err;

#define objfmt(name)	\
	if ((err = exec_load_##name(read, read_exec, handle, out_info)) != EX_NOT_EXECUTABLE)	\
		return err;
#include "objfmts.h"
#undef objfmt

	return EX_NOT_EXECUTABLE;
}

