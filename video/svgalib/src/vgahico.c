/* vgahico.c - used by ET4000 driver. */
/* Set Hicolor RAMDAC mode.

 * HH: Added support for 24-bit Sierra RAMDAC (15025/6) (untested).
 */


#include "vga.h"
#include "libvga.h"
#include "driver.h"
#include "ramdac/ramdac.h"

int __svgalib_hicolor(int dac_type, int mode)
/* Enters hicolor mode - 0 for no hi, 1 for 15 bit, 2 for 16, 3 for 24 */
/* For any modes it doesn't know about, etc, it attempts to turn hicolor off. */
{
    int x;

    _ramdac_dactocomm();
    x = port_in(PEL_MSK);
    _ramdac_dactocomm();
    switch (dac_type & ~1) {
    case 0:			/* Sierra SC11486 */
	if (mode == HI15_DAC)
	    port_out(x | 0x80, PEL_MSK);
	else
	    port_out(x & ~0x80, PEL_MSK);
	break;
    case 2:			/* Sierra Mark2/Mark3 */
	switch (mode) {
	case HI15_DAC:
	    /* port_out( (x | 0x80) & ~0x40, PEL_MSK); */
	    port_out((x & 0x1f) | 0xa0, PEL_MSK);
	    break;
	case HI16_DAC:
	    /* port_out( x | 0xC0, PEL_MSK); */
	    port_out((x & 0x1f) | 0xe0, PEL_MSK);
	    break;
	default:
	    port_out(x & ~0xC0, PEL_MSK);
	    break;
	}
	break;
    case 4:			/* Diamond SS2410 */
	if (mode == HI15_DAC)
	    port_out(x | 0x80, PEL_MSK);
	else
	    port_out(x & ~0x80, PEL_MSK);
	break;
    case 8:			/* AT&T 20c491/2 */
	switch (mode) {
	case HI15_DAC:
	    port_out((x & 0x1f) | 0xA0, PEL_MSK);
	    break;
	case HI16_DAC:
	    port_out((x & 0x1f) | 0xC0, PEL_MSK);
	    break;
	case TC24_DAC:
	    port_out((x & 0x1f) | 0xE0, PEL_MSK);
	    break;
	default:
	    port_out(x & 0x1F, PEL_MSK);
	    break;
	}
	break;
    case 16:			/* AcuMos ADAC1 */
	switch (mode) {
	case HI15_DAC:
	    port_out(0xF0, PEL_MSK);
	    break;
	case HI16_DAC:
	    port_out(0xE1, PEL_MSK);
	    break;
	case TC24_DAC:
	    port_out(0xE5, PEL_MSK);
	    break;
	default:
	    port_out(0, PEL_MSK);
	    break;
	}
	break;
    case 32:			/* Sierra 15025/6 24-bit DAC */
	switch (mode) {
	case HI15_DAC:
	    port_out((x & 0x1f) | 0xa0, PEL_MSK);
	    /* 0xa0 is also a 15-bit mode. */
	    break;
	case HI16_DAC:
	    port_out((x & 0x1f) | 0xe0, PEL_MSK);
	    /* 0xc0 is also a 16-bit mode. */
	    /* 0xc0 doesn't seem to work, so use 0xe0. */
	    break;
	case TC24_DAC:
	    port_out((x & 0x1f) | 0x60, PEL_MSK);
	    break;
	default:
	    port_out(x & 0x1f, PEL_MSK);
	    break;
	}
	break;
    default:
	/* Normal VGA mode. */
	port_out(x & 0x1F, PEL_MSK);
	break;
    }
    _ramdac_dactopel();

    return 0;
}
