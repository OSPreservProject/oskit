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
 * This was generated via:
 *

i486-linux-nm oskit_x11video.a | grep -v s3_ symbols | grep " T " | awk '{ printf "#define "$3" OSKIT_X11_"$3"\n" }' | sort > global.h

*
* Note that if the .a file is a.out then you will need to remove leading
* underscores.
*/
#ifndef _OSKIT_X11_GLOBAL_H_
#define _OSKIT_X11_GLOBAL_H_

#define ARK2000gendacSetClock OSKIT_X11_ARK2000gendacSetClock
#define AddScreen OSKIT_X11_AddScreen
#define AllocateScreenPrivateIndex OSKIT_X11_AllocateScreenPrivateIndex
#define AltICD2061SetClock OSKIT_X11_AltICD2061SetClock
#define Att409SetClock OSKIT_X11_Att409SetClock
#define BusToMem OSKIT_X11_BusToMem
#define Chrontel8391SetClock OSKIT_X11_Chrontel8391SetClock
#define ET4000gendacSetClock OSKIT_X11_ET4000gendacSetClock
#define ET4000gendacSetpixmuxClock OSKIT_X11_ET4000gendacSetpixmuxClock
#define ET4000stg1703SetClock OSKIT_X11_ET4000stg1703SetClock
#define ET6000SetClock OSKIT_X11_ET6000SetClock
#define ErrorF OSKIT_X11_ErrorF
#define Et4000AltICD2061SetClock OSKIT_X11_Et4000AltICD2061SetClock
#define FatalError OSKIT_X11_FatalError
#define FreeScreen OSKIT_X11_FreeScreen
#define GlennsIODelay OSKIT_X11_GlennsIODelay
#define IBMRGBSetClock OSKIT_X11_IBMRGBSetClock
#define ICD2061ACalcClock OSKIT_X11_ICD2061ACalcClock
#define ICD2061AGCD OSKIT_X11_ICD2061AGCD
#define ICD2061AGetClock OSKIT_X11_ICD2061AGetClock
#define ICD2061ASetClock OSKIT_X11_ICD2061ASetClock
#define ICS2595SetClock OSKIT_X11_ICS2595SetClock
#define ICS5342SetClock OSKIT_X11_ICS5342SetClock
#define InitOutput OSKIT_X11_InitOutput
#define MemToBus OSKIT_X11_MemToBus
#define NoopDDA OSKIT_X11_NoopDDA
#define NotImplemented OSKIT_X11_NotImplemented
#define ReplyNotSwappd OSKIT_X11_ReplyNotSwappd
#define ResetScreenPrivates OSKIT_X11_ResetScreenPrivates
#define S3AuroraSetClock OSKIT_X11_S3AuroraSetClock
#define S3Trio64V2SetClock OSKIT_X11_S3Trio64V2SetClock
#define S3TrioSetClock OSKIT_X11_S3TrioSetClock
#define S3ViRGE_VXSetClock OSKIT_X11_S3ViRGE_VXSetClock
#define S3gendacSetClock OSKIT_X11_S3gendacSetClock
#define SC11412SetClock OSKIT_X11_SC11412SetClock
#define STG1703CalcProgramWord OSKIT_X11_STG1703CalcProgramWord
#define STG1703SetClock OSKIT_X11_STG1703SetClock
#define STG1703getIndex OSKIT_X11_STG1703getIndex
#define STG1703magic OSKIT_X11_STG1703magic
#define STG1703setIndex OSKIT_X11_STG1703setIndex
#define SlowBcopy OSKIT_X11_SlowBcopy
#define StrCaseCmp OSKIT_X11_StrCaseCmp
#define StrToUL OSKIT_X11_StrToUL
#define Ti3025CalcNMP OSKIT_X11_Ti3025CalcNMP
#define Ti3025SetClock OSKIT_X11_Ti3025SetClock
#define Ti3026SetClock OSKIT_X11_Ti3026SetClock
#define Ti3030SetClock OSKIT_X11_Ti3030SetClock
#define VErrorF OSKIT_X11_VErrorF
#define Xalloc OSKIT_X11_Xalloc
#define Xcalloc OSKIT_X11_Xcalloc
#define Xfree OSKIT_X11_Xfree
#define Xrealloc OSKIT_X11_Xrealloc
#define commonCalcClock OSKIT_X11_commonCalcClock
#define debug_alloca OSKIT_X11_debug_alloca
#define debug_dealloca OSKIT_X11_debug_dealloca
#define find_bios_string OSKIT_X11_find_bios_string
#define gendacMNToClock OSKIT_X11_gendacMNToClock
#define newmmio_s3AdjustFrame OSKIT_X11_newmmio_s3AdjustFrame
#define newmmio_s3CloseScreen OSKIT_X11_newmmio_s3CloseScreen
#define newmmio_s3EnterLeaveVT OSKIT_X11_newmmio_s3EnterLeaveVT
#define newmmio_s3ImageInit OSKIT_X11_newmmio_s3ImageInit
#define newmmio_s3ImageWriteNoMem OSKIT_X11_newmmio_s3ImageWriteNoMem
#define newmmio_s3Initialize OSKIT_X11_newmmio_s3Initialize
#define newmmio_s3RestoreColor0 OSKIT_X11_newmmio_s3RestoreColor0
#define newmmio_s3RestoreDACvalues OSKIT_X11_newmmio_s3RestoreDACvalues
#define newmmio_s3SaveScreen OSKIT_X11_newmmio_s3SaveScreen
#define newmmio_s3ScreenInit OSKIT_X11_newmmio_s3ScreenInit
#define newmmio_s3SetVidPage OSKIT_X11_newmmio_s3SetVidPage
#define newmmio_s3SwitchMode OSKIT_X11_newmmio_s3SwitchMode
#define pciReadByte OSKIT_X11_pciReadByte
#define pciReadWord OSKIT_X11_pciReadWord
#define pciWriteByte OSKIT_X11_pciWriteByte
#define pciWriteWord OSKIT_X11_pciWriteWord
#define pcibusRead OSKIT_X11_pcibusRead
#define pcibusTag OSKIT_X11_pcibusTag
#define pcibusWrite OSKIT_X11_pcibusWrite
#define s3CleanUp OSKIT_X11_s3CleanUp
#define s3DetectELSA OSKIT_X11_s3DetectELSA
#define s3GetPCIInfo OSKIT_X11_s3GetPCIInfo
#define s3IBMRGB_Init OSKIT_X11_s3IBMRGB_Init
#define s3IBMRGB_Probe OSKIT_X11_s3IBMRGB_Probe
#define s3InBtReg OSKIT_X11_s3InBtReg
#define s3InBtRegCom3 OSKIT_X11_s3InBtRegCom3
#define s3InBtStatReg OSKIT_X11_s3InBtStatReg
#define s3InIBMRGBIndReg OSKIT_X11_s3InIBMRGBIndReg
#define s3InTi3026IndReg OSKIT_X11_s3InTi3026IndReg
#define s3InTiIndReg OSKIT_X11_s3InTiIndReg
#define s3Init OSKIT_X11_s3Init
#define s3InitEnvironment OSKIT_X11_s3InitEnvironment
#define s3OutBtReg OSKIT_X11_s3OutBtReg
#define s3OutBtRegCom3 OSKIT_X11_s3OutBtRegCom3
#define s3OutIBMRGBIndReg OSKIT_X11_s3OutIBMRGBIndReg
#define s3OutTi3026IndReg OSKIT_X11_s3OutTi3026IndReg
#define s3OutTiIndReg OSKIT_X11_s3OutTiIndReg
#define s3PrintIdent OSKIT_X11_s3PrintIdent
#define s3Probe OSKIT_X11_s3Probe
#define s3ReadDAC OSKIT_X11_s3ReadDAC
#define s3Unlock OSKIT_X11_s3Unlock
#define s3WriteDAC OSKIT_X11_s3WriteDAC
#define s3set3020mode OSKIT_X11_s3set3020mode
#define s3set485mode OSKIT_X11_s3set485mode
#define vgaDPMSSet OSKIT_X11_vgaDPMSSet
#define vgaGetClocks OSKIT_X11_vgaGetClocks
#define vgaHWInit OSKIT_X11_vgaHWInit
#define vgaHWRestore OSKIT_X11_vgaHWRestore
#define vgaHWSave OSKIT_X11_vgaHWSave
#define vgaHWSaveScreen OSKIT_X11_vgaHWSaveScreen
#define vgaProtect OSKIT_X11_vgaProtect
#define vgaSaveScreen OSKIT_X11_vgaSaveScreen
#define x11_main OSKIT_X11_x11_main
#define xf86AddIOPorts OSKIT_X11_xf86AddIOPorts
#define xf86CheckMode OSKIT_X11_xf86CheckMode
#define xf86ClearIOPortList OSKIT_X11_xf86ClearIOPortList
#define xf86CloseConsole OSKIT_X11_xf86CloseConsole
#define xf86Config OSKIT_X11_xf86Config
#define xf86ConfigError OSKIT_X11_xf86ConfigError
#define xf86DCConfigError OSKIT_X11_xf86DCConfigError
#define xf86DCGetOption OSKIT_X11_xf86DCGetOption
#define xf86DCGetToken OSKIT_X11_xf86DCGetToken
#define xf86DeleteMode OSKIT_X11_xf86DeleteMode
#define xf86DisableIOPorts OSKIT_X11_xf86DisableIOPorts
#define xf86DisableIOPrivs OSKIT_X11_xf86DisableIOPrivs
#define xf86DisableInterrupts OSKIT_X11_xf86DisableInterrupts
#define xf86EnableIOPorts OSKIT_X11_xf86EnableIOPorts
#define xf86EnableInterrupts OSKIT_X11_xf86EnableInterrupts
#define xf86GetClocks OSKIT_X11_xf86GetClocks
#define xf86GetNearestClock OSKIT_X11_xf86GetNearestClock
#define xf86GetPathElem OSKIT_X11_xf86GetPathElem
#define xf86GetToken OSKIT_X11_xf86GetToken
#define xf86InitViewport OSKIT_X11_xf86InitViewport
#define xf86LinearVidMem OSKIT_X11_xf86LinearVidMem
#define xf86LookupMode OSKIT_X11_xf86LookupMode
#define xf86MapDisplay OSKIT_X11_xf86MapDisplay
#define xf86MapVidMem OSKIT_X11_xf86MapVidMem
#define xf86OpenConsole OSKIT_X11_xf86OpenConsole
#define xf86ProcessArgument OSKIT_X11_xf86ProcessArgument
#define xf86ReadBIOS OSKIT_X11_xf86ReadBIOS
#define xf86StringToToken OSKIT_X11_xf86StringToToken
#define xf86TokenToString OSKIT_X11_xf86TokenToString
#define xf86UnMapDisplay OSKIT_X11_xf86UnMapDisplay
#define xf86UnMapVidMem OSKIT_X11_xf86UnMapVidMem
#define xf86UseMsg OSKIT_X11_xf86UseMsg
#define xf86VerifyOptions OSKIT_X11_xf86VerifyOptions
#define xf86cleanpci OSKIT_X11_xf86cleanpci
#define xf86clrdaccommbit OSKIT_X11_xf86clrdaccommbit
#define xf86dactocomm OSKIT_X11_xf86dactocomm
#define xf86dactopel OSKIT_X11_xf86dactopel
#define xf86getdaccomm OSKIT_X11_xf86getdaccomm
#define xf86getdacregindexed OSKIT_X11_xf86getdacregindexed
#define xf86scanpci OSKIT_X11_xf86scanpci
#define xf86setdaccomm OSKIT_X11_xf86setdaccomm
#define xf86setdaccommbit OSKIT_X11_xf86setdaccommbit
#define xf86setdacregindexed OSKIT_X11_xf86setdacregindexed
#define xf86testdacindexed OSKIT_X11_xf86testdacindexed
#define xf86writepci OSKIT_X11_xf86writepci

#endif /* _OSKIT_X11_GLOBAL_H_ */
