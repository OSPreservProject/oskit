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
 * This file simply lists the current set of Linux SCSI drivers.
 * This list is used for building liboskit_linux_dev itself,
 * by <oskit/dev/linux.h> to declare all the function prototypes,
 * and can be used by clients of the OSKIT as well
 * as an automated way to stay up-to-date with the supported drivers.
 *
 * Each driver is listed on a separate line in a standard format,
 * which can be parsed by the C preprocessor or other text tools like awk:
 *
 * driver(name, description, vendor, author, filename, template)
 *
 * name is the abbreviated name of the device, e.g., 'aha1542'.
 * description is the longer device name, e.g., 'Adaptec 1542'.
 * vendor is the name of the hardware vendor supplying this device.
 * author is the name of the author(s) of the device driver.
 * filename is the base filename of the Linux device driver.
 * template is the name of the Linux SCSI host template macro.
 */
#ifdef OSKIT_ARM32_SHARK

#else
driver(ncr53c8xx, "NCR53c8xx (rel 1.12d)", "NCR", "Wolfgang Stanglmeier", "ncr53c8xx", NCR53C8XX)
driver(ncr53c7xx, "NCR53c{7,8}xx (rel 17)", "NCR", "Drew Eckhardt", "53c7,8xx", NCR53c7xx)
driver(am53c974, "AM53C974", NULL, "Drew Eckhardt, Robin Cutshaw, D. Frieauff", "AM53C974", AM53C974)
driver(buslogic, "BusLogic", "BusLogic", "Leonard N. Zubkoff", "BusLogic", BUSLOGIC)
driver(ncr53c406a, "NCR53c406a", "NCR", "Normunds Saumanis", "NCR53c406a", NCR53c406a)
driver(advansys, "AdvanSys", "AdvanSys", "Advanced System Products, Inc.", "advansys", ADVANSYS)
driver(aha152x, "Adaptec 152x", "Adaptec", "Juergen E. Fischer, Rickard E. Faith", "aha152x", AHA152X)
driver(aha1542, "Adaptec 1542", "Adaptec", "Tommy Thorn, Eric Youngdale", "aha1542", AHA1542)
driver(aha1740, "Adaptec 174x (EISA)", "Adaptec", "Brad McLean", "aha1740", AHA1740)
driver(aic7xxx, "Adaptec AIC7xxx", "Adaptec", "John Aycock", "aic7xxx", AIC7XXX)
driver(dtc, "DTC 3180/3280", "Data Technology Corporation", "Ray Van Tassle, Drew Eckhardt", "dtc", DTC3x80)
driver(eata, "EATA/DMA 2.0x", NULL, "Dario Ballabio", "eata", EATA)
driver(eatadma, "EATA (Extended Attachment) HBA", NULL, "Michael Neuffer", "eata_dma", EATA_DMA)
driver(eatapio, "EATA (Extended Attachment) PIO", NULL, "Michael Neuffer, Alfred Arnold", "eata_pio", EATA_PIO)
driver(fdomain, "Future Domain TMC-16x0", "Future Domain", "Rickard E. Faith", "fdomain", FDOMAIN_16X0)
driver(in2000, "Always IN2000", "Always", "John Shifflett, GeoLog Consulting", "in2000", IN2000)
driver(pas16, "Pro Audio Spectrum-16", NULL, "John Weidman, Drew Eckhardt", "pas16", MV_PAS16)
driver(qlogicfas, "Qlogic FAS408", "Qlogic", "Tom Zerucha, Michael A. Griffith", "qlogicfas", QLOGICFAS)
driver(qlogicisp, "QLogic ISP1020 (PCI)", "Qlogic", "Erik H. Moe", "qlogicisp", QLOGICISP)
driver(seagate, "Seagate ST01/ST02, Future Domain TMC-885", "Seagate", "Drew Eckhardt", "seagate", SEAGATE_ST0X)
driver(t128, "Trantor T128/T128F/T228", "Trantor", "Drew Eckhardt", "t128", TRANTOR_T128)
driver(u14f34f, "UltraStor 14F/34F", "UltraStor", "Dario Ballabio", "u14-34f", ULTRASTOR_14_34F)
driver(ultrastor, "UltraStor 14F/24F/34F", "UltraStor", "David B. Gentzel, Whitfield Software Services", "ultrastor", ULTRASTOR_14F)
driver(wd7000, "Western Digital WD-7000", "Western Digital", "Thomas Wuensche, Tommy Thorn, John Boyd", "wd7000", WD7000)
#endif
