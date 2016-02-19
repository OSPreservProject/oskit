/*
 * Shark-specific DMA.  Just use ISA code.
 * XXX THIS IS NOT AN ORIGINAL LINUX FILE XXX
 * mike slam-hacked this from:
 *
 * arch/arm/kernel/dma-ebsa285.c
 *
 * Copyright (C) 1998 Phil Blundell
 *
 * DMA functions specific to EBSA-285/CATS architectures
 *
 * Changelog:
 *  09/11/1998	RMK	Split out ISA DMA functions to dma-isa.c
 */

#ifdef OSKIT
#include <linux/sched.h>
#include <linux/init.h>

#include <asm/dma.h>
#include <asm/io.h>
#include <asm/hardware.h>

#include "dma.h"
#include "dma-isa.h"

int arch_request_dma(dmach_t channel, dma_t *dma, const char *dev_name)
{
	return isa_request_dma(channel, dma, dev_name);
}

void arch_free_dma(dmach_t channel, dma_t *dma)
{
	isa_free_dma(channel, dma);
}

int arch_get_dma_residue(dmach_t channel, dma_t *dma)
{
	return isa_get_dma_residue(channel, dma);
}

void arch_enable_dma(dmach_t channel, dma_t *dma)
{
	isa_enable_dma(channel, dma);
}

void arch_disable_dma(dmach_t channel, dma_t *dma)
{
	isa_disable_dma(channel, dma);
}

__initfunc(void arch_dma_init(dma_t *dma))
{
	isa_init_dma(dma);
}
#endif
