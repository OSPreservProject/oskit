/*
 * Automatically generated C config: don't edit
 * [Automatically generated only on native Linux systems;
 * in the oskit you may edit this file.]
 */
#define AUTOCONF_INCLUDED 1

/*
 * Code maturity level options
 */
#define CONFIG_EXPERIMENTAL 1

/*
 * Loadable module support
 */
#undef CONFIG_MODULES

/*
 * General setup
 */
#undef  CONFIG_MATH_EMULATION
#define CONFIG_NET 1
#undef  CONFIG_MAX_16M
#ifndef OSKIT_ARM32_SHARK
#define CONFIG_PCI 1
#define CONFIG_PCI_OPTIMIZE 1
#endif
#undef  CONFIG_SYSVIPC
#undef  CONFIG_BINFMT_AOUT
#undef  CONFIG_BINFMT_ELF
#undef  CONFIG_BINFMT_JAVA
#undef  CONFIG_KERNEL_ELF
#undef  CONFIG_M386
#define CONFIG_M486 1
#undef  CONFIG_M586
#undef  CONFIG_M686

/*
 * Floppy, IDE, and other block devices
 */
#ifndef OSKIT_ARM32_SHARK
#define CONFIG_BLK_DEV_FD 1
#endif
#define CONFIG_BLK_DEV_IDE 1

/*
 * Please see Documentation/ide.txt for help/info on IDE drives
 */
#define CONFIG_BLK_DEV_IDEDISK 1
#undef  CONFIG_BLK_DEV_HD_IDE
#define CONFIG_BLK_DEV_IDEPCI 1
#define CONFIG_IDEDMA_AUTO 1
#ifndef OSKIT_ARM32_SHARK
#define CONFIG_BLK_DEV_IDECD 1
#endif
#undef  CONFIG_BLK_DEV_IDETAPE
#undef  CONFIG_BLK_DEV_IDE_PCMCIA
#define CONFIG_BLK_DEV_CMD640 1
#undef  CONFIG_BLK_DEV_CMD640_ENHANCED
#define CONFIG_BLK_DEV_RZ1000 1
#define CONFIG_BLK_DEV_IDEDMA 1
#define CONFIG_BLK_DEV_IDEFLOPPY 1
#undef  CONFIG_IDE_CHIPSETS

/*
 * Note: most of these also require special kernel boot parameters
 */
#undef  CONFIG_BLK_DEV_ALI14XX
#undef  CONFIG_BLK_DEV_DTC2278
#undef  CONFIG_BLK_DEV_HT6560B
#undef  CONFIG_BLK_DEV_PROMISE
#undef  CONFIG_BLK_DEV_QD6580
#undef  CONFIG_BLK_DEV_UMC8672

/*
 * Additional Block Devices
 */
#undef  CONFIG_BLK_DEV_LOOP
#undef  CONFIG_BLK_DEV_MD
#undef  CONFIG_BLK_DEV_RAM
#undef  CONFIG_BLK_DEV_XD
#undef  CONFIG_BLK_DEV_HD

/*
 * Networking options
 */
#undef  CONFIG_FIREWALL
#undef  CONFIG_NET_ALIAS
#undef  CONFIG_INET
#undef  CONFIG_IP_FORWARD
#undef  CONFIG_IP_MULTICAST
#undef  CONFIG_IP_ACCT

/*
 * (it is safe to leave these untouched)
 */
#undef  CONFIG_INET_PCTCP
#undef  CONFIG_INET_RARP
#undef  CONFIG_NO_PATH_MTU_DISCOVERY
#define CONFIG_IP_NOSR 1
#define CONFIG_SKB_LARGE 1

/*
 *
 */
#undef  CONFIG_IPX
#undef  CONFIG_ATALK
#undef  CONFIG_AX25
#undef  CONFIG_BRIDGE
#undef  CONFIG_NETLINK

/*
 * SCSI support
 */
#ifndef OSKIT_ARM32_SHARK
#define  CONFIG_SCSI 1
#endif

/*
 * SCSI support type (disk, tape, CD-ROM)
 */
#ifndef OSKIT_ARM32_SHARK
#define CONFIG_BLK_DEV_SD 1
#endif
#undef  CONFIG_CHR_DEV_ST
#undef  CONFIG_BLK_DEV_SR
#undef  CONFIG_CHR_DEV_SG

/*
 * Some SCSI devices (e.g. CD jukebox) support multiple LUNs
 */
#define CONFIG_SCSI_MULTI_LUN 1
#define CONFIG_SCSI_CONSTANTS 1

/*
 * SCSI low-level drivers
 */
#undef  CONFIG_SCSI_7000FASST
#undef  CONFIG_SCSI_AHA152X
#undef  CONFIG_SCSI_AHA1542
#undef  CONFIG_SCSI_AHA1740
#undef  CONFIG_SCSI_AIC7XXX
#undef  CONFIG_SCSI_ADVANSYS
#undef  CONFIG_SCSI_IN2000
#undef  CONFIG_SCSI_AM53C974
#undef  CONFIG_SCSI_BUSLOGIC
#undef  CONFIG_SCSI_DTC3280
#undef  CONFIG_SCSI_EATA_DMA
#undef  CONFIG_SCSI_EATA_PIO
#undef  CONFIG_SCSI_EATA
#undef  CONFIG_SCSI_FUTURE_DOMAIN
#undef  CONFIG_SCSI_GENERIC_NCR5380
#undef  CONFIG_SCSI_GENERIC_NCR53C400
#undef  CONFIG_SCSI_G_NCR5380_PORT
#undef  CONFIG_SCSI_G_NCR5380_MEM
#undef  CONFIG_SCSI_NCR53C406A
/* Linux-native NCR driver */
#undef  CONFIG_SCSI_NCR53C7xx
#undef  CONFIG_SCSI_NCR53C7xx_sync
#undef  CONFIG_SCSI_NCR53C7xx_FAST
#undef  CONFIG_SCSI_NCR53C7xx_DISCONNECT
/* FreeBSD-ported NCR driver */
#undef  CONFIG_SCSI_NCR53C8XX
#undef  CONFIG_SCSI_NCR53C8XX_TAGGED_QUEUE
#undef  CONFIG_SCSI_NCR53C8XX_IOMAPPED
#undef  CONFIG_SCSI_NCR53C8XX_NO_DISCONNECT
#undef  CONFIG_SCSI_NCR53C8XX_FORCE_ASYNCHRONOUS
#undef  CONFIG_SCSI_NCR53C8XX_FORCE_SYNC_NEGO
#undef  CONFIG_SCSI_NCR53C8XX_DISABLE_MPARITY_CHECK
#undef  CONFIG_SCSI_NCR53C8XX_DISABLE_PARITY_CHECK
#undef  CONFIG_SCSI_PPA
#undef  CONFIG_SCSI_PAS16
#undef  CONFIG_SCSI_QLOGIC_FAS
#undef  CONFIG_SCSI_QLOGIC_ISP
#undef  CONFIG_SCSI_SEAGATE
#undef  CONFIG_SCSI_T128
#undef  CONFIG_SCSI_U14_34F
#undef  CONFIG_SCSI_ULTRASTOR

/*
 * Network device support
 */
#define CONFIG_NETDEVICES 1
#undef  CONFIG_DUMMY
#undef  CONFIG_EQUALIZER
#undef  CONFIG_DLCI
#undef  CONFIG_PLIP
#undef  CONFIG_PPP
#undef  CONFIG_SLIP
#define CONFIG_NET_RADIO 1
#undef  CONFIG_BAYCOM
#undef  CONFIG_STRIP
#undef  CONFIG_WAVELAN
#undef  CONFIG_WIC
#undef  CONFIG_SCC
#define CONFIG_NET_ETHERNET 1
#define CONFIG_NET_VENDOR_3COM 1
#undef  CONFIG_EL1
#define CONFIG_EL2 1
#undef  CONFIG_ELPLUS
#undef  CONFIG_EL16
#define CONFIG_EL3 1
#define CONFIG_VORTEX 1
#define CONFIG_LANCE 1
#define CONFIG_LANCE32 1
#define CONFIG_NET_VENDOR_SMC 1
#undef  CONFIG_WD80x3
#define CONFIG_ULTRA 1
#undef  CONFIG_SMC9194
#define CONFIG_NET_ISA 1
#undef  CONFIG_AT1700
#undef  CONFIG_E2100
#undef  CONFIG_DEPCA
#define CONFIG_EWRK3 1
#undef  CONFIG_EEXPRESS
#undef  CONFIG_EEXPRESS_PRO
#undef  CONFIG_EEPRO100		/* alpha driver avail -- untested */
#undef  CONFIG_FMV18X
#define CONFIG_HPLAN_PLUS 1
#undef  CONFIG_HPLAN
#undef  CONFIG_HP100
#undef  CONFIG_ETH16I
#define CONFIG_NE2000 1
#undef  CONFIG_NI52
#undef  CONFIG_NI65
#undef  CONFIG_SEEQ8005
#undef  CONFIG_SK_G16
#define CONFIG_NET_EISA 1
#undef  CONFIG_AC3200
#undef  CONFIG_APRICOT
#undef  CONFIG_DE4X5
#define CONFIG_DEC_ELCP 1
#undef  CONFIG_DGRS
#undef  CONFIG_ZNET
#undef  CONFIG_NET_POCKET
#undef  CONFIG_ATP
#undef  CONFIG_DE600
#undef  CONFIG_DE620
#undef  CONFIG_TR
#undef  CONFIG_IBMTR
#undef  CONFIG_FDDI
#undef  CONFIG_DEFXX
#undef  CONFIG_ARCNET
#undef  CONFIG_ARCNET_ETH
#undef  CONFIG_ARCNET_1051

/*
 * ISDN subsystem
 * Completly untested
 */
#undef  CONFIG_ISDN
#undef  CONFIG_ISDN_PPP
#undef  CONFIG_ISDN_AUDIO
#undef  CONFIG_ISDN_DRV_ICN
#undef  CONFIG_ISDN_DRV_PCBIT
#undef  CONFIG_ISDN_DRV_TELES

/*
 * CD-ROM drivers (not for SCSI or IDE/ATAPI drives)
 */
#define CONFIG_CD_NO_IDESCSI 1
#undef  CONFIG_AZTCD
#undef  CONFIG_GSCD
#undef  CONFIG_SBPCD
#undef  CONFIG_MCD
#undef  CONFIG_MCDX
#undef  CONFIG_OPTCD
#undef  CONFIG_CM206
#undef  CONFIG_SJCD
#undef  CONFIG_CDI_INIT
#undef  CONFIG_CDU31A
#undef  CONFIG_CDU535

/*
 * Filesystems
 * We don't use Linux filesystems...
 */
#undef  CONFIG_QUOTA
#undef  CONFIG_LOCK_MANDATORY
#undef  CONFIG_MINIX_FS
#undef  CONFIG_EXT_FS
#undef  CONFIG_EXT2_FS
#undef  CONFIG_XIA_FS
#undef  CONFIG_FAT_FS
#undef  CONFIG_MSDOS_FS
#undef  CONFIG_VFAT_FS
#undef  CONFIG_UMSDOS_FS
#undef  CONFIG_PROC_FS
#undef  CONFIG_NFS_FS
#undef  CONFIG_ROOT_NFS
#undef  CONFIG_SMB_FS
#undef  CONFIG_ISO9660_FS
#undef  CONFIG_HPFS_FS
#undef  CONFIG_SYSV_FS
#undef  CONFIG_AFFS_FS
#undef  CONFIG_UFS_FS

/*
 * Character devices
 * Not currently supported in the OSKit
 */
#define CONFIG_SERIAL 1
#undef  CONFIG_DIGI
#undef  CONFIG_CYCLADES
#undef  CONFIG_STALDRV
#undef  CONFIG_RISCOM8
#undef  CONFIG_PRINTER
#undef  CONFIG_MOUSE
#undef  CONFIG_UMISC
#undef  CONFIG_QIC02_TAPE
#undef  CONFIG_FTAPE
#undef  CONFIG_APM
#undef  CONFIG_WATCHDOG
#undef  CONFIG_RTC

/*
 * Sound
 * Only works on the Shark right now
 */
#ifdef OSKIT_ARM32_SHARK
#define CONFIG_SOUND		1
#define CONFIG_SOUND_OSS	1
#define CONFIG_SOUND_SB		1
#define CONFIG_SOUND_SBDSP	1
#define CONFIG_SB_BASE		0x220	/* XXX shouldn't be here */
#define CONFIG_SB_IRQ		9	/* XXX shouldn't be here */
#else
#undef  CONFIG_SOUND
#endif

/*
 * Kernel hacking
 */
#undef  CONFIG_PROFILE
