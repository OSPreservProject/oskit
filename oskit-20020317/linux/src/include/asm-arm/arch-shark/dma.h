/*
 * linux/include/asm-arm/arch-shark/dma.h
 *
 * by Alexander.Schulz@stud.uni-karlsruhe.de
 */
#ifndef __ASM_ARCH_DMA_H
#define __ASM_ARCH_DMA_H

/* Use only the lowest 4MB, nothing else works.
 * The rest is not DMAable. See dev /  .properties
 * in OpenFirmware.
 */
#ifdef OSKIT
#define MAX_DMA_ADDRESS		0x18400000
#else
#define MAX_DMA_ADDRESS		0xC0400000
#endif
#define MAX_DMA_CHANNELS	8
#define DMA_ISA_CASCADE		4

#endif /* _ASM_ARCH_DMA_H */
