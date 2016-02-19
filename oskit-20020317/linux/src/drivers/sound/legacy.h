#ifndef _SOUND_LEGACY_H_
#define _SOUND_LEGACY_H_

/*
 *	Force on additional support
 */

#define __SGNXPRO__
#define DESKPROXL
/* #define SM_GAMES */
#define SM_WAVE

/*
 * Define legacy options.
 */

#define SELECTED_SOUND_OPTIONS		0x0

#undef HAVE_MAUI_BOOT
#undef PSS_HAVE_LD
#undef INCLUDE_TRIX_BOOT

#undef CONFIG_CS4232
#undef CONFIG_GUS
#undef CONFIG_MAD16
#undef CONFIG_MAUI
#undef CONFIG_MPU401
#undef CONFIG_MSS
#undef CONFIG_OPL3SA1
#undef CONFIG_OPL3SA2
#undef CONFIG_PAS
#undef CONFIG_PSS
#define CONFIG_SB
#undef CONFIG_SOFTOSS
#undef CONFIG_SSCAPE
#undef CONFIG_AD1816
#undef CONFIG_TRIX
#undef CONFIG_VMIDI
#undef CONFIG_YM3812

#define CONFIG_AUDIO
#undef CONFIG_MIDI
#undef CONFIG_SEQUENCER

#undef CONFIG_AD1848
#undef CONFIG_MPU_EMU
#define CONFIG_SBDSP
#define CONFIG_UART401

#endif	/* _SOUND_LEGACY_H */
