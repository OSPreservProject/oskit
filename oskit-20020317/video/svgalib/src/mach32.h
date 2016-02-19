/* VGAlib version 1.2 - (c) 1993 Tommy Frandsen                    */
/*                                                                 */
/* This library is free software; you can redistribute it and/or   */
/* modify it without any restrictions. This library is distributed */
/* in the hope that it will be useful, but without any warranty.   */

/* ATI Mach32 driver (C) 1994 Michael Weller                       */
/* Silly defines for convenience application communication with    */
/* Mach32 Driver.                                                  */

#ifndef _MACH32_H
#define _MACH32_H

/* Valid params for chiptype in mach32_init(int force, int chiptype, int memory);
   Combine with | */

#define MACH32_FORCE_APERTURE	 0x80	/* Force aperture to type given.. */
#define MACH32_APERTURE		 0x20	/* Aperture is available */
#define MACH32_APERTURE_4M	 0x40	/* Aperture has 4Megs (and is available ) */


#define MACH32_FORCEDAC		 0x10	/* Set dac to type specified below: */
/* Known Dac-Types */
#define MACH32_ATI68830	0
#define MACH32_SC11483	1
#define MACH32_ATI6871	2
#define MACH32_BT476	3
#define MACH32_BT481	4
#define MACH32_ATI68860 5

#endif				/* _MACH32_H */
