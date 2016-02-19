
#include <sys/types.h>

/*
 * Magic number when image is compressed
 */
#define COMPRESSED_MAGIC	0x69696969

/*
 * Each compressed block of the file has this little header on it.
 * Since each subblock is independently compressed, we need to know
 * its internal size (it will probably be shorter than 1MB) since we
 * have to know exactly how much to give the inflator.
 */
struct blockhdr {
	int		magic;
	unsigned long   size;		/* Size of compressed part */
	int		blockindex;
	int		blocktotal;
	int		regionsize;
	int		regioncount;
};

/*
 * This little struct defines the pair. Each number is in sectors. An array
 * of these come after the header above, and is padded to a 1K boundry.
 * The region says where to write the next part of the input file, which is
 * how we skip over parts of the disk that do not need to be written
 * (swap, free FS blocks).
 */
struct region {
	unsigned long	start;
	unsigned long	size;
};

/*
 * In the new model, each sub region has its own region header info.
 * But there is no easy way to tell how many regions before compressing.
 * Just leave a page, and hope that 512 regions is enough!
 *
 * This number must be a multiple of the NFS read size in netdisk.
 */
#define DEFAULTREGIONSIZE	(1024 * 4)

/*
 * Ah, the frisbee protocol. The new world order is to break up the
 * file into fixed 1MB chunks, with the region info prepended to each
 * chunk so that it can be layed down on disk independently of all the
 * chunks in the file. 
 */
#define SUBBLOCKSIZE		(1024 * 1024)
#define SUBBLOCKMAX		(SUBBLOCKSIZE - DEFAULTREGIONSIZE)
