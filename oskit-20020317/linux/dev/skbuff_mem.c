/*
 * Copyright (c) 1996-2000 The University of Utah and the Flux Group.
 * 
 * This file is part of the OSKit Linux Glue Libraries, which are free
 * software, also known as "open source;" you can redistribute them and/or
 * modify them under the terms of the GNU General Public License (GPL),
 * version 2, as published by the Free Software Foundation (FSF).
 * 
 * The OSKit is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GPL for more details.  You should have
 * received a copy of the GPL along with the OSKit; see the file COPYING.  If
 * not, write to the FSF, 59 Temple Place #330, Boston, MA 02111-1307, USA.
 */
#include <oskit/c/assert.h>

#include <linux/skbuff.h>
#include <linux/malloc.h>		/* kmalloc/free */

/*
 * SKB cache.  Cache three (data) sizes of SKBs.
 * Zero is easy.  Anything <= 200 bytes gets a 200 byte SKB, which is cached.
 * The other size is full sized ether SKBs.  Unfortunately, the drivers tend
 * to add slack to the actual MTU, so look for 1500-1550 bytes.
 */
#define SMALL_SKB_SIZE	200
#define SMALL_SKB(x)	((x) > 0 && (x) <= 200)
#define LARGE_SKB_SIZE	1600
#define LARGE_SKB(x)	((x) >= 1500 && (x) <= 1600)

#define MAX_ZERO_SKBS	100	/* These cutoffs are totally arbitrary! */
#define MAX_SMALL_SKBS	300
#define MAX_LARGE_SKBS	20

static struct sk_buff	*zero_skbs;
static struct sk_buff	*small_skbs;
static struct sk_buff	*large_skbs;
static int		zero_skb_count, small_skb_count, large_skb_count;

/* #define SKB_CACHE_STATS */
#ifdef SKB_CACHE_STATS
struct skb_cache_stats {
	int		zero_hits, zero_misses;
	int		small_hits, small_misses;
	int		large_hits, large_misses;
	int		bad_size;
};
static struct skb_cache_stats skb_cache_stats;
#define SKB_CACHE_STAT(x) (skb_cache_stats. x)
#else
#define SKB_CACHE_STAT(x)
#endif

/*
 * Allocate a buffer for use as an skbuff.
 * Returns a pointer to a buffer of at least the indicated size.  Also returns
 * the actual size of the buffer allocated.  Any additional space returned may
 * be used by the caller.  The caller should present the same size when freeing
 * the buffer.  Flags are OSKIT_MEM_ (aka OSENV_) allocation flags used to
 * indicate special memory needs.
 *
 * This routine may be called at hardware interrupt time and should protect
 * itself as necessary.
 */
void *
oskit_skbufio_mem_alloc(oskit_size_t size, int mflags, oskit_size_t *out_size)
{
	struct sk_buff *skb;
	unsigned long flags;

	assert(size >= SKB_HDRSIZE);
	assert(mflags == OSENV_NONBLOCKING);
	assert(out_size != 0);

	size -= SKB_HDRSIZE;

	flags = linux_save_flags_cli();
	if (size == 0) {
		if ((skb = zero_skbs) != NULL) {
			zero_skbs = zero_skbs->next;
			zero_skb_count--;
			SKB_CACHE_STAT(zero_hits++);

			goto gotone;
		}
		SKB_CACHE_STAT(zero_misses++);
	}
	else if (SMALL_SKB(size)) {
		size = SMALL_SKB_SIZE;
		if ((skb = small_skbs) != NULL) {
			small_skbs = small_skbs->next;
			small_skb_count--;
			SKB_CACHE_STAT(small_hits++);
			goto gotone;
		}
		SKB_CACHE_STAT(small_misses++);
	}
	else if (LARGE_SKB(size)) {
		size = LARGE_SKB_SIZE;
		if ((skb = large_skbs) != NULL) {
			large_skbs = large_skbs->next;
			large_skb_count--;
			SKB_CACHE_STAT(large_hits++);
			goto gotone;
		}
		SKB_CACHE_STAT(large_misses++);
	}
	else {
		SKB_CACHE_STAT(bad_size++);
	}

	skb = kmalloc(SKB_HDRSIZE + size, GFP_ATOMIC);

 gotone:
	linux_restore_flags(flags);
	*out_size = SKB_HDRSIZE + size;
	return skb;
}

void
oskit_skbufio_mem_free(void *ptr, oskit_size_t size)
{
	struct sk_buff *skb = ptr;
	unsigned long flags;

	assert(size >= SKB_HDRSIZE);
	size -= SKB_HDRSIZE;

	/*
	 * Look for cacheble SKBs before letting them go completely
	 */
	flags = linux_save_flags_cli();
	if (size == 0 && zero_skb_count < MAX_ZERO_SKBS) {
		skb->next = zero_skbs;
		zero_skbs = skb;
		zero_skb_count++;
	}
	else if (size == SMALL_SKB_SIZE &&
		 small_skb_count < MAX_SMALL_SKBS) {
		skb->next = small_skbs;
		small_skbs = skb;
		small_skb_count++;
	}
	else if (size == LARGE_SKB_SIZE &&
		 large_skb_count < MAX_LARGE_SKBS) {
		skb->next = large_skbs;
		large_skbs = skb;
		large_skb_count++;
	}
	else {
		kfree(skb);
	}
	linux_restore_flags(flags);
}

#ifdef SKB_CACHE_STATS
#include <oskit/c/stdio.h>

void
oskit_linux_skbmem_dump(void)
{
	printf("sk_buff cache: %d hits, %d misses (%d bad size)\n",
	       skb_cache_stats.zero_hits+skb_cache_stats.small_hits+
	       skb_cache_stats.large_hits,
	       skb_cache_stats.zero_misses+skb_cache_stats.small_misses+
	       skb_cache_stats.large_misses, skb_cache_stats.bad_size);
	printf("zero: %d hits, %d misses\n",
	       skb_cache_stats.zero_hits, skb_cache_stats.zero_misses);
	printf("small: %d hits, %d misses\n",
	       skb_cache_stats.small_hits, skb_cache_stats.small_misses);
	printf("large: %d hits, %d misses\n",
	       skb_cache_stats.large_hits, skb_cache_stats.large_misses);
}
#endif
