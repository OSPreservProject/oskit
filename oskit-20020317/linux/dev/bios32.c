/*
 * Copyright (c) 1997, 1998, 1999 The University of Utah and the Flux Group.
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
 * This provides the BIOS32 PCI interfaces for use by the Linux
 * device drivers.  It is based entirely on word reads and writes
 * to PCI configuration space.
 */

#include <linux/config.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/bios32.h>
#include <linux/pci.h>

#ifdef CONFIG_PCI

#include "osenv.h"

/* In Linux: device_fn = device << 3 | function */

/* This is the highest bus number we find */
static int max_pci_bus = 1;

/*
 * This is machine-independant (PCI-specific) code to deal with
 * PCI configuration space.
 *
 * This deals with multi-functon devices and PCI-PCI bridges.
 * Every PCI-PCI bridge incrementes the number of PCI buses by 1.
 * A note about multi-function-devices: if a device is NOT a MFD,
 * it doesn't have to decode the function ID.  So we have to
 * check.  If it is a MFD, it must fully decode the function ID,
 * but the function IDs can be used sparsely.  However, every
 * valid PCI device must use at least function 0.
 *
 * The underlying osenv_pci_config_read() function must be able
 * to generate the configuration-space special cycle required.
 * This is machine-dependant.
 */


int
pcibios_present(void)
{
	return !osenv_pci_config_init();
}

int
pcibios_find_class(unsigned int class_code, unsigned short index,
	unsigned char *bus, unsigned char *device_fn)
{
	int found = -1;
	unsigned mybus, device, function;
	unsigned my_id;
	int num_fun;
	char type;
	int bridge_count = 0;

	/* In Linux: device_fn = device << 3 | function */

	/*
	 * Since I don't keep a table, I have to start at the beginning
	 * every time...
	 */

	for (mybus = 0; mybus < max_pci_bus; mybus ++)
		for (device = 0; device < 32; device ++) {
			/* If invalid device/function 0 skip */
			/* function 0 must always be used if device is there */
			osenv_pci_config_read(mybus, device, 0, 0, &my_id);
			if ((my_id == -1) || (my_id == 0))
				continue;

			/* see if multi-function device */
			pcibios_read_config_byte(mybus, device << 3, 14, &type);
			if (type & 0x80)
				num_fun = 8;
			else
				num_fun = 1;

			for (function = 0; function < num_fun; function++) {
				osenv_pci_config_read(mybus, device, function,
					8, &my_id);
				if ((my_id == -1) || (my_id == 0))
					continue;

				/* if PCI-PCI bridge, increment bus count */
				if ((my_id >> 16) == 0x0604) {
					bridge_count++;
					if (bridge_count == max_pci_bus) {
						printk(KERN_DEBUG"PCI bridge found: PCI%d:%d:%d\n",
							mybus, device, function);
						max_pci_bus++;
					}
				}
				my_id >>= 8;

				/* Note: if there is a device, function 0 must
				 * have a valid vendor (!= 0xffff)
				 */
				if (my_id == class_code) {
					found ++;
					if (found == index) {
						if (num_fun > 1)
							osenv_log(OSENV_LOG_DEBUG, "MFD at %d:%d\n",
								mybus, device);
						*bus = mybus;
						*device_fn = device << 3 | function;
						return PCIBIOS_SUCCESSFUL;
					}
				}
			}
		}

	/* PCIBIOS_SUCCESSFUL (0) success, PCI BIOS error code otherwise */
	return PCIBIOS_DEVICE_NOT_FOUND;
}


int
pcibios_find_device(unsigned short vendor, unsigned short device_id,
	unsigned short index, unsigned char *bus, unsigned char *device_fn)
{
	int found = -1;
	unsigned mybus, device, function;
	unsigned my_id;
	int num_fun;
	char type;
	int bridge_count = 0;

	unsigned ven_dev = (device_id << 16) | vendor;

	/*
	 * Since I don't keep a table, I have to start at the beginning
	 * every time...
	 */

	for (mybus = 0; mybus < max_pci_bus; mybus ++)
		for (device = 0; device < 32; device ++) {
			/* If invalid device/function 0 skip */
			/* function 0 must always be used if device is there */
			osenv_pci_config_read(mybus, device, 0, 0, &my_id);
			if ((my_id == -1) || (my_id == 0))
				continue;

			/* see if multi-function device */
			pcibios_read_config_byte(mybus, device << 3, 14, &type);
			if (type & 0x80)
				num_fun = 8;
			else
				num_fun = 1;
			for (function = 0; function < num_fun; function++) {

				/* if PCI-PCI bridge, increment bus count */
				osenv_pci_config_read(mybus, device, function,
					8, &my_id);
				if ((my_id >> 16) == 0x0604) {
					bridge_count++;
					if (bridge_count == max_pci_bus) {
						printk(KERN_DEBUG"PCI bridge found: PCI%d:%d:%d\n",
							mybus, device, function);
						max_pci_bus++;
					}
				}

				osenv_pci_config_read(mybus, device, function,
					0, &my_id);

				/* Note: if there is a device, function 0 must
				 * have a valid vendor (!= 0xffff)
				 */

				if (my_id == ven_dev) {
					found ++;
					if (found == index) {
						if (num_fun > 1)
							osenv_log(OSENV_LOG_DEBUG, "MFD at %d:%d\n",
								mybus, device);
						*bus = mybus;
						*device_fn = device << 3 | function;
						return PCIBIOS_SUCCESSFUL;
					}
				}
			}
		}

	/* PCIBIOS_SUCCESSFUL (0) on success */
	return PCIBIOS_DEVICE_NOT_FOUND;
}

#undef pcibios_read_config_byte
int
pcibios_read_config_byte(unsigned char bus, unsigned char device_fn,
	unsigned char where, unsigned char *value)
{
	int tmp, rc;

	rc = osenv_pci_config_read(bus, device_fn >> 3, device_fn & 0x7,
		where & ~0x3, &tmp);

	if (rc)
		return rc;

	switch (where & 0x3) {
	case 0:
		*value = tmp & 0xff;
		break;
	case 1:
		*value = (tmp >> 8) & 0xff;
		break;
	case 2:
		*value = (tmp >> 16) & 0xff;
		break;
	case 3:
		*value = (tmp >> 24) & 0xff;
		break;
	}
	return 0;
}

int
pcibios_read_config_word(unsigned char bus, unsigned char device_fn,
	unsigned char where, unsigned short *value)
{
	unsigned val;
	int rc;
	osenv_assert((where & 0x1) == 0);

	/* word aligned */
	rc = osenv_pci_config_read(bus, device_fn >> 3, device_fn & 0x7,
		where & ~0x3, &val);
	if (rc)
		return rc;

	if (where & 0x2)
		*value = (val >> 16) & 0xffff;
	else
		*value = val & 0xffff;

	return 0;
}

int
pcibios_read_config_dword(unsigned char bus, unsigned char device_fn,
	unsigned char where, unsigned int *value)
{
	return osenv_pci_config_read(bus, device_fn >> 3, device_fn & 0x7,
		where, value);
}

int
pcibios_write_config_byte(unsigned char bus, unsigned char device_fn,
	unsigned char where, unsigned char value)
{
	unsigned tmp;

	osenv_pci_config_read(bus, device_fn >> 3, device_fn & 0x7,
		where & ~0x3, &tmp);

	switch (where & 0x3) {
	case 0:
		tmp = (tmp & 0xffffff00) | value;
		break;
	case 1:
		tmp = (tmp & 0xffff00ff) | (value << 8);
		break;
	case 2:
		tmp = (tmp & 0xff00ffff) | (value << 16);
		break;
	case 3:
		tmp = (tmp & 0x00ffffff) | (value << 24);
		break;
	}
	osenv_pci_config_write(bus, device_fn >> 3, device_fn & 0x7,
		where & ~0x3, value);
	return 0;
}

int
pcibios_write_config_word(unsigned char bus, unsigned char device_fn,
	unsigned char where, unsigned short value)
{
	unsigned tmp;

	osenv_assert((where & 0x1) == 0);
	osenv_pci_config_read(bus, device_fn >> 3, device_fn & 0x7,
		where & ~0x3, &tmp);

	if (where & 0x2) {
		tmp = (tmp & 0x0000ffff) | ((long)value << 16);
	} else {
		tmp = (tmp & 0xffff0000) | value;
	}
	osenv_pci_config_write(bus, device_fn >> 3, device_fn & 0x7,
		where & ~3, tmp);
	return 0;
}

int
pcibios_write_config_dword(unsigned char bus,
	unsigned char device_fn, unsigned char where, unsigned int value)
{
	return osenv_pci_config_write(bus, device_fn >> 3, device_fn & 0x7,
		where, value);
}


/*
 * This code was lifted from Linux
 */
const char *
pcibios_strerror(int error)
{
	static char buf[80];

	if (error)
		printk(KERN_ERR"PCIB error %x\n", error);

	switch (error) {
		case PCIBIOS_SUCCESSFUL:
			return "SUCCESSFUL";

		case PCIBIOS_FUNC_NOT_SUPPORTED:
			return "FUNC_NOT_SUPPORTED";

		case PCIBIOS_BAD_VENDOR_ID:
			return "SUCCESSFUL";

		case PCIBIOS_DEVICE_NOT_FOUND:
			return "DEVICE_NOT_FOUND";

		case PCIBIOS_BAD_REGISTER_NUMBER:
			return "BAD_REGISTER_NUMBER";

		case PCIBIOS_SET_FAILED:
			return "SET_FAILED";

		case PCIBIOS_BUFFER_TOO_SMALL:
			return "BUFFER_TOO_SMALL";

		default:
			sprintf (buf, "UNKNOWN RETURN 0x%x", error);
			return buf;
	}
}


void
pcibios_fixup(void)
{
}

void
pcibios_fixup_bus(struct pci_bus *bus)
{

}
#endif /* CONFIG_PCI */


void
pcibios_init(void)
{
#ifdef CONFIG_PCI
	printk(KERN_DEBUG"pcibios_init : OSKit PCI BIOS revision 1.0\n");
#endif
}


char *
pcibios_setup(char *str)
{
#ifdef CONFIG_PCI
	printk(KERN_DEBUG"pcibios_setup : %s\n", str);
#endif
	return 0;
}
