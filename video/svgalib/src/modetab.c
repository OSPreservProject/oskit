/*
   ** modetab.c  -  part of svgalib
   ** (C) 1993 by Hartmut Schirmer
   **
   ** A modetable holds a pair of values 
   ** for each mode :
   **
   **    <mode number> <pointer to registers>
   **
   ** the last entry is marked by 
   **  
   **    <any number>  <NULL>
 */

#include <stdlib.h>
#include "driver.h"

const unsigned char *
 __svgalib_mode_in_table(const ModeTable * modes, int mode)
{
    while (modes->regs != NULL) {
	if (modes->mode_number == mode)
	    return modes->regs;
	modes++;
    }
    return NULL;
}
