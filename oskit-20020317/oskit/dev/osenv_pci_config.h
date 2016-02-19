/*
 * Copyright (c) 1997-1999 University of Utah and the Flux Group.
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
 * COMification of the osenv pci configuration interface. 
 */
#ifndef _OSKIT_DEV_OSENV_PCI_CONFIG_H_
#define _OSKIT_DEV_OSENV_PCI_CONFIG_H_

#include <oskit/com.h>
#include <oskit/error.h>
#include <oskit/dev/dev.h>
#include <oskit/dev/driver.h>

/*
 * osenv pci configuration interface.
 *
 * IID 4aa7dff2-7c74-11cf-b500-08000953adc2
 */
struct oskit_osenv_pci_config {
	struct oskit_osenv_pci_config_ops *ops;
};
typedef struct oskit_osenv_pci_config oskit_osenv_pci_config_t;

struct oskit_osenv_pci_config_ops {

	/*** COM-specified IUnknown interface operations ***/
	OSKIT_COMDECL_IUNKNOWN(oskit_osenv_pci_config_t)

	OSKIT_COMDECL   (*init)(oskit_osenv_pci_config_t *o);

	OSKIT_COMDECL_U (*read)(oskit_osenv_pci_config_t *o,
				char bus, char device, char function,
				char port, unsigned *data);

	OSKIT_COMDECL_U (*write)(oskit_osenv_pci_config_t *o,
				char bus, char device, char function,
				char port, unsigned data);
};

/* GUID for oskit_osenv_pci_config interface */
extern const struct oskit_guid oskit_osenv_pci_config_iid;
#define OSKIT_OSENV_PCI_CONFIG_IID OSKIT_GUID(0x4aa7dff2, 0x7c74, 0x11cf, \
				0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#define oskit_osenv_pci_config_query(s, iid, out_ihandle) \
	((s)->ops->query((oskit_osenv_pci_config_t *)(s), \
			 (iid), (out_ihandle)))
#define oskit_osenv_pci_config_addref(s) \
	((s)->ops->addref((oskit_osenv_pci_config_t *)(s)))
#define oskit_osenv_pci_config_release(s) \
	((s)->ops->release((oskit_osenv_pci_config_t *)(s)))

#define oskit_osenv_pci_config_init(s) \
	((s)->ops->init((oskit_osenv_pci_config_t *)(s)))
#define oskit_osenv_pci_config_read(s, b, d, f, p, data) \
	((s)->ops->read((oskit_osenv_pci_config_t *)(s), \
			(b), (d), (f), (p), (data)))
#define oskit_osenv_pci_config_write(s, b, d, f, p, data) \
	((s)->ops->write((oskit_osenv_pci_config_t *)(s), \
			 (b), (d), (f), (p), (data)))

/*
 * Return a reference to an osenv pci configuration interface object.
 */
oskit_osenv_pci_config_t	*oskit_create_osenv_pci_config(void);

#endif /* _OSKIT_DEV_OSENV_PCI_CONFIG_H_ */
