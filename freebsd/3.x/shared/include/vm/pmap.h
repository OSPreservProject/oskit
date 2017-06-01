/* Just a fake, to satisfy bogus #include's */

/*
 * Each machine dependent implementation is expected to
 * keep certain statistics.  They may do this anyway they
 * so choose, but are expected to return the statistics
 * in the following structure.
 */
struct pmap_statistics {
	long resident_count;	/* # of pages mapped (total) */
	long wired_count;	/* # of pages wired */
};
typedef struct pmap_statistics *pmap_statistics_t;

#include <machine/pmap.h>

