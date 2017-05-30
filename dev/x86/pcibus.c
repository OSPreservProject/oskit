/*
 * Copyright (c) 1998 University of Utah and the Flux Group.
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
 * There is no hook for generating a special cycle, as those
 * are generally used to signal events such as halt/shutdown.
 * Should it be required by a driver, it is possible to add
 * support for special cycles.
 * Note that a driver could program a special cycle manually if
 * it is given access to the controller.
 *
 * This code is based on pcibus.c from FreeBSD 2.2.5
 * and the header file, pcivar.h
 */

typedef unsigned long u_long;
typedef unsigned short u_short;
typedef unsigned char u_char;

int pci_maxdevice;
int pci_mechanism;

#define printf(args...) osenv_log(OSENV_LOG_INFO, args)
#define panic(args...) osenv_panic(args)

/* Luckily, we use outb(port,val) in oskit */
#include <oskit/x86/pio.h>

#include <oskit/dev/dev.h>

#define bootverbose 0
#undef PCI_DEBUG

/**************************************************************************
**
**  $\Id: pcivar.h,v 1.13 1996/09/10 23:31:02 bde Exp $
**
**  Declarations for pci device drivers.
**
**  FreeBSD
**
**-------------------------------------------------------------------------
**
** Copyright (c) 1994 Wolfgang Stanglmeier.  All rights reserved.
**
***************************************************************************
*/

typedef union {
	u_long cfg1;
	struct {
		u_char  enable;
		u_char  forward;
		u_short port;
	} cfg2;
	unsigned tag;
} pcici_t;

#define sametag(x,y)  ((x).tag == (y).tag)


/**************************************************************************
**
**  $\Id: pcibus.c,v 1.27 1996/10/30 22:38:55 asami Exp $
**
**  pci bus subroutines for i386 architecture.
**
**  FreeBSD
**
**-------------------------------------------------------------------------
**
** Copyright (c) 1994 Wolfgang Stanglmeier.  All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
***************************************************************************
*/


/*-----------------------------------------------------------------
**
**	The following functions are provided by the pci bios.
**	They are used only by the pci configuration.
**
**	pcibus_setup():
**		Probes for a pci system.
**		Sets pci_maxdevice and pci_mechanism.
**
**	pcibus_tag():
**		Creates a handle for pci configuration space access.
**		This handle is given to the read/write functions.
**
**	pcibus_read():
**		Read a long word from the pci configuration space.
**		Requires a tag (from pcitag) and the register
**		number (should be a long word alligned one).
**
**	pcibus_write():
**		Writes a long word to the pci configuration space.
**		Requires a tag (from pcitag), the register number
**		(should be a long word alligned one), and a value.
**
**-----------------------------------------------------------------
*/

static int
pcibus_check (void);

static void
pcibus_setup (void);

static pcici_t
pcibus_tag (u_char bus, u_char device, u_char func);

static u_long
pcibus_read (pcici_t tag, u_long reg);

static void
pcibus_write (pcici_t tag, u_long reg, u_long data);


/*--------------------------------------------------------------------
**
**      Determine configuration mode
**
**--------------------------------------------------------------------
*/


#define CONF1_ADDR_PORT    0x0cf8
#define CONF1_DATA_PORT    0x0cfc

#define CONF1_ENABLE       0x80000000ul
#define CONF1_ENABLE_CHK   0x80000000ul
#define CONF1_ENABLE_MSK   0x7ff00000ul
#define CONF1_ENABLE_CHK1  0xff000001ul
#define CONF1_ENABLE_MSK1  0x80000001ul
#define CONF1_ENABLE_RES1  0x80000000ul

#define CONF2_ENABLE_PORT  0x0cf8
#ifdef PC98
#define CONF2_FORWARD_PORT 0x0cf9
#else
#define CONF2_FORWARD_PORT 0x0cfa
#endif

#define CONF2_ENABLE_CHK   0x0e
#define CONF2_ENABLE_RES   0x0e

static int
pcibus_check (void)
{
	u_char device;

	if (bootverbose) printf ("pcibus_check:\tdevice ");

	for (device = 0; device < pci_maxdevice; device++) {
		unsigned long id;
		if (bootverbose)
			printf ("%d ", device);
		id = pcibus_read (pcibus_tag (0,device,0), 0);
		if (id && id != 0xfffffffful) {
			if (bootverbose) printf ("is there (id=%08lx)\n", id);
			return 1;
		}
	}
	if (bootverbose)
		printf ("-- nothing found\n");
	return 0;
}

static void
pcibus_setup (void)
{
	unsigned long mode1res,oldval1;
	unsigned char mode2res,oldval2;

	if (osenv_io_avail(CONF1_ADDR_PORT, 4) == 0) {
		printf("can't alloc CONF1_ADDR_PORT\n");
		return;
	}
	if (osenv_io_avail(CONF1_DATA_PORT, 4) == 0) {
		printf("can't alloc CONF1_DATA_PORT\n");
		return;
	}

	oldval1 = inl (CONF1_ADDR_PORT);

	if (bootverbose) {
		printf ("pcibus_setup(1):\tmode 1 addr port (0x0cf8) is 0x%08lx\n", oldval1);
	}

	/*---------------------------------------
	**      Assume configuration mechanism 1 for now ...
	**---------------------------------------
	*/

	if ((oldval1 & CONF1_ENABLE_MSK) == 0) {

		pci_mechanism = 1;
		pci_maxdevice = 32;

		outl (CONF1_ADDR_PORT, CONF1_ENABLE_CHK);
		outb (CONF1_ADDR_PORT +3, 0);
		mode1res = inl (CONF1_ADDR_PORT);
		outl (CONF1_ADDR_PORT, oldval1);

		if (bootverbose)
		    printf ("pcibus_setup(1a):\tmode1res=0x%08lx (0x%08lx)\n",
			    mode1res, CONF1_ENABLE_CHK);

		if (mode1res) {
			if (pcibus_check())
				return;
		};

		outl (CONF1_ADDR_PORT, CONF1_ENABLE_CHK1);
		mode1res = inl(CONF1_ADDR_PORT);
		outl (CONF1_ADDR_PORT, oldval1);

		if (bootverbose)
		    printf ("pcibus_setup(1b):\tmode1res=0x%08lx (0x%08lx)\n",
			    mode1res, CONF1_ENABLE_CHK1);

		if ((mode1res & CONF1_ENABLE_MSK1) == CONF1_ENABLE_RES1) {
			if (pcibus_check())
				return;
		};
	}

	/*---------------------------------------
	**      Try configuration mechanism 2 ...
	**---------------------------------------
	*/
	if (osenv_io_avail(CONF2_ENABLE_PORT, 1) == 0) {
		printf("can't alloc CONF2_ENABLE_PORT\n");
		return;
	}
	if (osenv_io_avail(CONF2_FORWARD_PORT, 1) == 0) {
		printf("can't alloc CONF2_FORWARD_PORT\n");
		return;
	}
	if (osenv_io_avail(0xc000, 0xfff) == 0) {
		printf("can't alloc CONF2 range\n");
	}


	oldval2 = inb (CONF2_ENABLE_PORT);

	if (bootverbose) {
		printf ("pcibus_setup(2):\tmode 2 enable port (0x0cf8) is 0x%02x\n", oldval2);
	}

	if ((oldval2 & 0xf0) == 0) {

		pci_mechanism = 2;
		pci_maxdevice = 16;

		outb (CONF2_ENABLE_PORT, CONF2_ENABLE_CHK);
		mode2res = inb(CONF2_ENABLE_PORT);
		outb (CONF2_ENABLE_PORT, oldval2);

		if (bootverbose)
		    printf ("pcibus_setup(2a):\tmode2res=0x%02x (0x%02x)\n",
			    mode2res, CONF2_ENABLE_CHK);

		if (mode2res == CONF2_ENABLE_RES) {
		    if (bootverbose)
			printf ("pcibus_setup(2a):\tnow trying mechanism 2\n");

			if (pcibus_check())
				return;
		}
	}

	/*---------------------------------------
	**      No PCI bus host bridge found
	**---------------------------------------
	*/

	pci_mechanism = 0;
	pci_maxdevice = 0;
}

/*--------------------------------------------------------------------
**
**      Build a pcitag from bus, device and function number
**
**--------------------------------------------------------------------
*/

static pcici_t
pcibus_tag (unsigned char bus, unsigned char device, unsigned char func)
{
	pcici_t tag;

	tag.cfg1 = 0;
	if (func   >=  8) return tag;

	switch (pci_mechanism) {

	case 1:
		if (device < 32) {
			tag.cfg1 = CONF1_ENABLE
				| (((u_long) bus   ) << 16ul)
				| (((u_long) device) << 11ul)
				| (((u_long) func  ) <<  8ul);
		}
		break;
	case 2:
		if (device < 16) {
			tag.cfg2.port    = 0xc000 | (device << 8ul);
			tag.cfg2.enable  = 0xf0 | (func << 1ul);
			tag.cfg2.forward = bus;
		}
		break;
	};
	return tag;
}

/*--------------------------------------------------------------------
**
**      Read register from configuration space.
**
**--------------------------------------------------------------------
*/

static u_long
pcibus_read (pcici_t tag, u_long reg)
{
	u_long addr, data = 0;

	if (!tag.cfg1) return (0xfffffffful);

	switch (pci_mechanism) {

	case 1:
		addr = tag.cfg1 | (reg & 0xfc);
#ifdef PCI_DEBUG
		printf ("pci_conf_read(1): addr=%x ", addr);
#endif
		outl (CONF1_ADDR_PORT, addr);
		data = inl (CONF1_DATA_PORT);
		outl (CONF1_ADDR_PORT, 0   );
		break;

	case 2:
		addr = tag.cfg2.port | (reg & 0xfc);
#ifdef PCI_DEBUG
		printf ("pci_conf_read(2): addr=%x ", addr);
#endif
		outb (CONF2_ENABLE_PORT , tag.cfg2.enable );
		outb (CONF2_FORWARD_PORT, tag.cfg2.forward);

		data = inl ((u_short) addr);

		outb (CONF2_ENABLE_PORT,  0);
		outb (CONF2_FORWARD_PORT, 0);
		break;
	};

#ifdef PCI_DEBUG
	printf ("data=%x\n", data);
#endif

	return (data);
}

/*--------------------------------------------------------------------
**
**      Write register into configuration space.
**
**--------------------------------------------------------------------
*/

static void
pcibus_write (pcici_t tag, u_long reg, u_long data)
{
	u_long addr;

	if (!tag.cfg1) return;

	switch (pci_mechanism) {

	case 1:
		addr = tag.cfg1 | (reg & 0xfc);
#ifdef PCI_DEBUG
		printf ("pci_conf_write(1): addr=%x data=%x\n",
			addr, data);
#endif
		outl (CONF1_ADDR_PORT, addr);
		outl (CONF1_DATA_PORT, data);
		outl (CONF1_ADDR_PORT,   0 );
		break;

	case 2:
		addr = tag.cfg2.port | (reg & 0xfc);
#ifdef PCI_DEBUG
		printf ("pci_conf_write(2): addr=%x data=%x\n",
			addr, data);
#endif
		outb (CONF2_ENABLE_PORT,  tag.cfg2.enable);
		outb (CONF2_FORWARD_PORT, tag.cfg2.forward);

		outl ((u_short) addr, data);

		outb (CONF2_ENABLE_PORT,  0);
		outb (CONF2_FORWARD_PORT, 0);
		break;
	};
}



/*
 * This provides the support code for the osenv access to PCI
 * configuration space.
 */
static int osenv_inited = 0;


int
pci_config_read(char bus, char device, char function, char port, unsigned *data)
{
	pcici_t tag;

	osenv_assert(osenv_inited);

	/* config mechanism may not support entire spec. */
	if (device >= pci_maxdevice)
		return 1;

	tag = pcibus_tag(bus, device, function);
	osenv_assert((port & 0x3) == 0);
	*data = pcibus_read(tag, port);

	return 0;
}

int
pci_config_write(char bus, char device, char function, char port, unsigned data)
{
	pcici_t tag;

	osenv_assert(osenv_inited);

	osenv_assert((port & 0x3) == 0);
	if (device >= pci_maxdevice)
		return 1;

	tag = pcibus_tag(bus, device, function);
	pcibus_write(tag, port, data);

	return 0; /* XXX */
}

int
pci_config_init()
{
	if (!osenv_inited) {
		pcibus_setup();
		osenv_inited = 1;
	}
	/* If PCI bus was found, pci_mechanism will be 1 or 2. */
	return (pci_mechanism == 0);
}
