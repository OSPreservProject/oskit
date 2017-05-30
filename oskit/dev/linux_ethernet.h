/*
 * Copyright (c) 1997-2000 University of Utah and the Flux Group.
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
 * This file simply lists the current set of Linux Ethernet drivers.
 * This list is used for building liboskit_linux_dev itself,
 * by <oskit/dev/linux.h> to declare all the function prototypes,
 * and can be used by clients of the OSKIT as well
 * as an automated way to stay up-to-date with the supported drivers.
 *
 * Each driver is listed on a separate line in a standard format,
 * which can be parsed by the C preprocessor or other text tools like awk:
 *
 * driver(name, description, vendor, author, filename, probe)
 *
 * name is the abbreviated name of the device, e.g., 'aha1542'.
 * description is the longer device name, e.g., 'Adaptec 1542'.
 * vendor is the name of the hardware vendor supplying this device.
 * author is the name of the author(s) of the device driver.
 * filename is the base filename of the Linux device driver.
 * probe is the name of the Linux probe function for this driver.
 *
 * These drivers are listed in the same order
 * in which they appear in Linux's drivers/net/Space.c file;
 * this is important as it typically determines probe order.
 * 
 * XXX: the above isn't strictly true, since linux keeps changing the 
 * probe order in new kernels.
 *
 */
#ifdef OSKIT_ARM32_SHARK
driver(cs89x0, "Crystal Semiconductor CS89[02]0", NULL, "Russell Nelson", "cs89x0", cs89x0_probe)
#else
driver(lance, "LANCE", "AMD", "Donald Becker", "lance", lance_probe)
driver(vortex, "3Com 3c590/3c595 \"Vortex\"", "3Com", "Donald Becker", "3c59x", tc59x_probe)
driver(epic100, "SMC EPIC/100 83C170", "SMC", "Donald Becker", "epic100", epic100_probe)
driver(seeq8005, "SEEQ 8005", NULL, "Donald Becker", "seeq8005", seeq8005_probe)
driver(tulip, "DEC 21040", "Digital Equipment Corporation", "Donald Becker", "tulip", tulip_probe)
driver(eepro100, "Intel i82557", "Intel", "Donald Becker", "eepro100", eepro100_probe)
driver(hp100, "Hewlett Packard HP10/100VG ANY LAN", "Hewlett Packard", "Jaroslav Kysela", "hp100", hp100_probe)
driver(ultra, "SMC Ultra and SMC EtherEZ ISA", "SMC", "Donald Becker", "smc-ultra", ultra_probe)
driver(smc9194, "SMC 9000 series", "SMC", "Erik Stahlman", "smc9194", smc_init)
driver(wd, "WD80x3", NULL, "Donald Becker", "wd", wd_probe)
driver(etherlink2, "3Com Etherlink 2", "3Com", "Donald Becker", "3c503", el2_probe)
driver(hp, "HP LAN", "Hewlett Packard", "Donald Becker", "hp", hp_probe)
driver(hpplus, "HP PC-LAN/plus", "Hewlett Packard", "Donald Becker", "hp-plus", hp_plus_probe)
driver(ac3200, "Ansel Communications EISA", "Ansel Communications", "Donald Becker", "ac3200", ac3200_probe)
driver(ne, "NE1000, NE2000", NULL, "Donald Becker", "ne", ne_probe)
/* XXX: This probe crashes our NE2000 cards, hanging the machine. */
/* Make sure it's called _after_ the ne probe. */
driver(e2100, "Cabletron E2100", "Cabletron", "Donald Becker", "e2100", e2100_probe)
driver(at1700, "Allied Telesis AT1700", "Allied Telesis", "Donald Becker", "at1700", at1700_probe)
driver(fmv18x, "Fujitsu FMV-181/182/183/184", "Fujitsu", "Donald Becker, Yutaka TAMIYA", "fmv18x", fmv18x_probe)
driver(eth16i, "ICL EtherTeam 16i and 32 EISA", "ICL", "Mika Kuoppala", "eth16i", eth16i_probe)
driver(etherlink3, "3Com EtherLink III", "3Com", "Donald Becker", "3c509", el3_probe)
driver(znet, "Zenith Z-Note", "Zenith", "Donald Becker", "znet", znet_probe)
driver(eexpress, "Intel EtherExpress", "Intel", "John Sullivan", "eexpress", express_probe)
driver(eepro, "Intel EtherExpress Pro/10", "Intel", "Bao C. Ha", "eepro", eepro_probe)
driver(ewrk3, "DIGITAL EtherWORKS 3", "Digital Equipment Corporation", "David C. Davies, Digital Equipment Corporation", "ewrk3", ewrk3_probe)
driver(de4x5, "DIGITAL DE425/DE434/DE435/DE450/DE500", "Digital Equipment Corporation", "David C. Davies, Digital Equipment Corporation", "de4x5", de4x5_probe)
driver(etherlink, "3Com Etherlink 3c501", "3Com", "Donald Becker", "3c501", el1_probe)
driver(etherlink16, "3Com Etherlink 16", "3Com", "Donald Becker", "3c507", el16_probe)
driver(etherlinkplus, "3Com Etherlink Plus", "3Com", "Craig Southeren, Juha Laiho and Philip Blundell", "3c505", elplus_probe)
driver(sk_g16, "SK G16", "Schneider & Koch", "Patrick J.D. Weichmann", "sk_g16", SK_init)
driver(ni52, "NI5210", NULL, "Michael Hipp", "ni52", ni52_probe)
driver(ni65, "NI6510", NULL, "Michael Hipp", "ni65", ni65_probe)
#endif
