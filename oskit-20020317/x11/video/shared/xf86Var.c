#include "xf86.h"
#include "xf86Procs.h"
#include "xf86_OSlib.h"

Bool xf86VTSema = FALSE;

/* dix/globals.c */

unsigned long serverGeneration = 1;
int monitorResolution = 0;
