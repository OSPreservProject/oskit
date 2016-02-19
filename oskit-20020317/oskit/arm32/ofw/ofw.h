/*
 * Copyright (c) 1999, 2001 University of Utah and the Flux Group.
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
 * OFW definitions,
 */

#include <oskit/types.h>

/*
 * OFW interface functions. These are hooks to the actual ROM routines.
 */
void		OF_init(int (*entrypoint)(void *));
void		OF_exit(void);
void		OF_boot(char *bootspec);
int		OF_finddevice(char *name);
int		OF_getprop(int handle, char *prop, void *buf, int buflen);
unsigned int	OF_decode_int(const unsigned char *p);
int		OF_read(int handle, void *buf, int buflen);
int		OF_write(int handle, void *buf, int buflen);
int		OF_getproplen(int handle, char *prop);
int		OF_call_method_1(char *method, int ihandle, int nargs, ...);
int		OF_call_method(char *method,
			int ihandle, int nargs, int nreturns, ...);
int		OF_instance_to_package(int ihandle);
int		OF_open(char *name);
int		OF_peer(int node);
int		OF_child(int node);

/*
 * OFW configuration functions. 
 */
void		ofw_configisa(void);
void		ofw_getphysmeminfo(void);
void		ofw_iomap(oskit_addr_t va, oskit_addr_t pa,
			oskit_size_t size, oskit_u32_t prot);
void		ofw_physmap(oskit_addr_t pa,
			oskit_size_t size, oskit_u32_t prot);
void		ofw_vmmap(oskit_addr_t va, oskit_addr_t pa,
			oskit_size_t size, oskit_u32_t prot);
int		ofw_vga_init(void);
oskit_addr_t	ofw_gettranslation(oskit_addr_t va);
oskit_addr_t	ofw_getcleaninfo(void);
void		ofw_paging_init(oskit_addr_t pdir_pa);
void		ofw_cacheclean_init(void);

/*
 * OFW structures.
 */

/*
 * OFW memory region structure.
 */
struct ofw_memregion {
	oskit_addr_t	start;
	oskit_size_t	size;
};

/*
 * OFW virtual memory translation structure.
 */
struct ofw_vtop {
	oskit_addr_t	virt;
	oskit_size_t	size;
	oskit_addr_t	phys;
	unsigned int	mode;
};

/*
 * OFW virtual memory translation structure.
 */
struct ofw_translation {
	oskit_addr_t	virt;
	oskit_size_t	size;
	oskit_addr_t	phys;
	unsigned int	mode;
};

