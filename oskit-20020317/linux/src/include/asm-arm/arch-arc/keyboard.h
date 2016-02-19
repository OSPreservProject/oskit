/*
 * linux/include/asm-arm/arch-arc/keyboard.h
 *
 * Keyboard driver definitions for Acorn Archimedes/A5000
 * architecture
 *
 * Copyright (C) 1998 Russell King
 */

#include <linux/ioport.h>
#include <asm/io.h>
#include <asm/irq.h>

#define NR_SCANCODES 128

extern void a5kkbd_leds(unsigned char leds);
extern void a5kkbd_init_hw(void);
extern unsigned char a5kkbd_sysrq_xlate[NR_SCANCODES];

#define kbd_setkeycode(sc,kc)		(-EINVAL)
#define kbd_getkeycode(sc)		(-EINVAL)

#define kbd_translate(sc, kcp, rm)	({ *(kcp) = (sc); 1; })
#define kbd_unexpected_up(kc)		(0200)
#define kbd_leds(leds)			a5kkbd_leds(leds)
#define kbd_init_hw()			a5kkbd_init_hw()
#define kbd_sysrq_xlate			a5kkbd_sysrq_xlate
#define kbd_disable_irq()		disable_irq(IRQ_KEYBOARDRX)
#define kbd_enable_irq()		enable_irq(IRQ_KEYBOARDRX)

#define SYSRQ_KEY	13

/* resource allocation */
#define KEYBOARD_IRQ                   1
#define kbd_request_region() request_region(0x60, 16, "keyboard")
#define kbd_request_irq(handler) request_irq(KEYBOARD_IRQ, handler, 0, \
                                             "keyboard", NULL)

/* How to access the keyboard macros on this platform.  */
#define kbd_read_input() inb(KBD_DATA_REG)
#define kbd_read_status() inb(KBD_STATUS_REG)
#define kbd_write_output(val) outb(val, KBD_DATA_REG)
#define kbd_write_command(val) outb(val, KBD_CNTL_REG)

/* Some stoneage hardware needs delays after some operations.  */
#define kbd_pause() do { } while(0)

/*
 * Machine specific bits for the PS/2 driver
 */

#define AUX_IRQ 12

#define aux_request_irq(hand, dev_id)					\
	request_irq(AUX_IRQ, hand, SA_SHIRQ, "PS/2 Mouse", dev_id)

#define aux_free_irq(dev_id) free_irq(AUX_IRQ, dev_id)
