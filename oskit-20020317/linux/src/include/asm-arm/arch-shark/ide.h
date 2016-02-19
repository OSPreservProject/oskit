/*
 * linux/include/asm-arm/arch-ebsa285/ide.h
 *
 * Copyright (c) 1998 Russell King
 *
 * Modifications:
 *  29-07-1998	RMK	Major re-work of IDE architecture specific code
 */
#include <asm/irq.h>

#ifdef OSKIT
typedef unsigned short ide_ioreg_t;

static __inline__ ide_ioreg_t
ide_default_io_base(int index)
{
	switch (index) {
		case 0:	return 0x1f0;
		case 1:	return 0x170;
		case 2: return 0x1e8;
		case 3: return 0x168;
		case 4: return 0x1e0;
		case 5: return 0x160;
		default:
			return 0;
	}
}

static __inline__ int
ide_default_irq(ide_ioreg_t base)
{
	switch (base) {
		case 0x1f0: return 14;
		default:
			return 0;
	}
}

static __inline__ void
ide_init_hwif_ports (ide_ioreg_t *p, ide_ioreg_t base, int *irq)
{
	ide_ioreg_t port = base;
	int i = 8;

	while (i--)
		*p++ = port++;
	*p++ = base + 0x206;
	if (irq != NULL)
		*irq = 0;
}
#else
/*
 * Set up a hw structure for a specified data port, control port and IRQ.
 * This should follow whatever the default interface uses.
 */
static __inline__ void
ide_init_hwif_ports(hw_regs_t *hw, int data_port, int ctrl_port, int irq)
{
	ide_ioreg_t reg = (ide_ioreg_t) data_port;
	int i;

	memset(hw, 0, sizeof(*hw));

	for (i = IDE_DATA_OFFSET; i <= IDE_STATUS_OFFSET; i++) {
		hw->io_ports[i] = reg;
		reg += 1;
	}
	hw->io_ports[IDE_CONTROL_OFFSET] = (ide_ioreg_t) ctrl_port;
	hw->irq = irq;
}
#endif

/*
 * This registers the standard ports for this architecture with the IDE
 * driver.
 */
static __inline__ void
ide_init_default_hwifs(void)
{
}
