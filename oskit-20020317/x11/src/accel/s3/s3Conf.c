/*
 * This file is generated automatically -- DO NOT EDIT
 */

#include "s3.h"

extern s3VideoChipRec
#ifdef OSKIT
	NEWMMIO;
#else
        NEWMMIO,
	MMIO_928,
        S3_GENERIC;
#endif 

s3VideoChipPtr s3Drivers[] =
{
        &NEWMMIO,
#ifndef OSKIT
	&MMIO_928,
        &S3_GENERIC,
#endif /* !OSKIT */
        NULL
};
