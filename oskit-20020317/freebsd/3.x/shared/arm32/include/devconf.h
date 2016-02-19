/*
 * Stub
 */
struct machdep_devconf {
};

#define MACHDEP_COPYDEV(dc, kdc) ((dc)->dc_md = (kdc)->kdc_md)
#define machdep_kdevconf machdep_devconf
