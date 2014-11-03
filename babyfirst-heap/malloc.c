/* 
  A version of malloc/free/realloc written by Doug Lea and released to the 
  public domain.  Send questions/comments/complaints/performance data
  to dl@cs.oswego.edu

  VERSION 2.6.1 Sat Dec  2 14:11:12 1995  Doug Lea  (dl at gee)

* Overview

  Vital statistics:

   Alignment:                            8-byte
   Assumed pointer representation:       4 bytes
   Assumed size_t  representation:       4 bytes
   Minimum wastage per allocated chunk:  4 bytes
   Maximum wastage per allocated chunk: 24 bytes 
   Minimum allocated size:              16 bytes (12 bytes usable, 4 overhead)
   Maximum allocated size:      2147483640 (2^31 - 8) bytes

   Explanations:

       Malloced chunks have space overhead of 4 bytes for the size
       field.  When a chunk is in use, only the `front' size is used,
       plus a bit in the NEXT adjacent chunk saying that its previous
       chunk is in use.

       When a chunk is freed, 12 additional bytes are needed; 4 for
       the trailing size field and 8 bytes for free list
       pointers. Thus, the minimum allocatable size is 16 bytes,
       of which 12 bytes are usable.

       8 byte alignment is currently hardwired into the design.  This
       seems to suffice for all current machines and C compilers.
       Calling memalign will return a chunk that is both 8-byte
       aligned and meets the requested (power of two) alignment.
       Alignnment demands, plus the minimum allocatable size restriction
       make the worst-case wastage 24 bytes. (Empirically, average
       wastage is around 5 to 7 bytes.)

       It is assumed that 32 bits suffice to represent chunk sizes.
       The maximum size chunk is 2^31 - 8 bytes.  

       malloc(0) returns a pointer to something of the minimum
       allocatable size.  Requests for negative sizes (when size_t is
       signed) or those greater than (2^31 - 8) bytes will also return
       a minimum-sized chunk. 

  Structure:

    This malloc, like any other, is a compromised design. 

    Chunks of memory are maintained using a `boundary tag' method as
    described in e.g., Knuth or Standish.  (See the paper by Paul
    Wilson ftp://ftp.cs.utexas.edu/pub/garbage/allocsrv.ps for a
    survey of such techniques.)  Sizes of free chunks are stored both
    in the front of each chunk and at the end.  This makes
    consolidating fragmented chunks into bigger chunks very fast.  The
    size fields also hold bits representing whether chunks are free or
    in use.

    Available chunks are kept in any of three places:

    * `av': An array of chunks serving as bin headers for consolidated
       chunks. Each bin is doubly linked.  The bins are approximately
       proportionally (log) spaced.  There are a lot of these bins too
       (128). This may look excessive, but works very well in
       practice.  All procedures maintain the invariant that no
       consolidated chunk physically borders another one. 

    * `top': The top-most available chunk (i.e., the one bordering the
       end of available memory) is treated specially. It is never
       included in any bin, is always kept fully consolidated, is used
       only if no other chunk is available, and is released back to
       the system if it is very large (> TRIM_THRESHOLD).

    * `last_remainder': A bin holding only the remainder of the
       most recently split (non-top) chunk. This bin is checked
       before other non-fitting chunks, so as to provide better
       locality for runs of sequentially allocated chunks. (However,
       it is only used in this way for `small' requests (< 512 bytes).
       For larger requests, getting better fits outweighs this.)


* Descriptions of public routines

  malloc:

    The requested size is first converted into a usable form, `nb'.
    This currently means to add 4 bytes overhead plus possibly more to
    obtain 8-byte alignment and/or to obtain a size of at least
    MINSIZE (currently 16 bytes), the smallest allocatable size.

    From there, there are five possible ways to allocate a chunk:

      1. For small requests (less than 512 bytes), the corresponding
         bin is scanned, and if a chunk of exactly the right size is
         found, it is taken.  

      2. For small requests only, the chunk that was split off
         as the most recent remaindered chunk is checked, and used if
         possible.

      3. Bins are scanned in increasing size order, using a chunk big
         enough to fulfill the request, and splitting off any
         remainder.  Bins >= BEST_FIT_THRESHOLD_BIN_NUMBER are searched
         exaustively, to find the best-fitting chunk. Others are
         scanned more quickly, in a way approximating LRU order.

      4. The chunk bordering the end of memory is split off.

      5. New space is obtained from the system using sbrk.  Memory is
         gathered from the system in a way that allows chunks obtained
         across different sbrk calls to be consolidated, but does not
         require contiguous memory. Thus, it should be safe to
         intersperse mallocs with other sbrk calls.

  free: 

    There are three cases:

       1. free(0) has no effect.  

       2. If a returned chunk borders the current high end of memory,
          it is consolidated into the top, and if the total unused
          topmost memory exceeds TRIM_THRESHOLD, malloc_trim is
          called. The default value of TRIM_THRESHOLD is high enough
          so that trimming should only occur if the program is
          maintaining enough unused memory to be worth releasing.

       3. Other chunks are consolidated as they arrive, and
          placed in corresponding bins.

  realloc:

    Reallocation proceeds in the usual way. If a chunk can be extended,
    it is, else a malloc-copy-free sequence is taken. 

    The old unix realloc convention of allowing the last-free'd chunk
    to be used as an argument to realloc is no longer supported.
    I don't know of any programs still relying on this feature,
    and allowing it would also allow too many other incorrect 
    usages of realloc to be sensible.

    Unless the #define REALLOC_ZERO_BYTES_FREES below is set,
    realloc with a size argument of zero allocates a minimum-sized
    chunk. 

  memalign:

    memalign requests more than enough space from malloc, finds a spot
    within that chunk that meets the alignment request, and then
    possibly frees the leading and trailing space. Overreliance on
    memalign is a sure way to fragment space.

  valloc:

    valloc just invokes memalign with alignment argument equal
    to the page size of the system (or as near to this as can
    be figured out from all the includes/defines below.)

  calloc:

    calloc calls malloc, then zeroes out the allocated chunk.

  cfree:

    cfree just calls free.

  malloc_trim:

    This routine gives memory back to the system (via negative
    arguments to sbrk) if there is unused memory at the `high' end of
    the malloc pool. You can call this after freeing large blocks of
    memory to potentially reduce the system-level memory requirements
    of a program. However, it cannot guarantee to reduce memory. Under
    some allocation patterns, some large free blocks of memory will be
    locked between two used chunks, so they cannot be given back to
    the system.

  malloc_usable_size:

    This routine tells you how many bytes you can actually use in
    an allocated chunk, which may be up to 24 bytes more than you
    requested (although typically much less; often 0). You can use
    this many bytes without worrying about overwriting other allocated
    objects. Not a particularly great programming practice, but still
    sometimes useful.

  malloc_stats:

    Prints on stderr the amount of space obtain from the system, the
    maximum amount (which may be more than current if malloc_trim got
    called), and the current number of bytes allocated via malloc (or
    realloc, etc) but not yet freed.
    

* Other implementation notes

  * Debugging:

    Because freed chunks may be overwritten with link fields, this
    malloc will often die when freed memory is overwritten by user
    programs.  This can be very effective (albeit in an annoying way)
    in helping users track down dangling pointers.

    If you compile with -DDEBUG, a number of assertion checks are enabled
    that will catch more memory errors. You probably won't be able to
    make much sense of the actual assertion errors, but they will
    at least tell you that you are overwriting memory that you ought
    not to be.  The checking is fairly extensive, and will slow down
    execution considerably. Calling malloc_stats with DEBUG set will
    attempt to check every allocated and free chunk in the course of
    computing the summmaries.

    Setting DEBUG may also be helpful if you are trying to modify 
    this code. The assertions in the check routines spell out in more 
    detail the assumptions and invariants underlying the code.

  * Performance differences from previous versions

    Users of malloc-2.5.X will find that generally, the current
    version conserves space better, especially when large chunks are
    allocated. For example, it wastes much less memory when user
    programs occasionally do things like like allocate space for GIF
    images amid other requests.  Since the current version does not
    delay consolidation, it is usually faster for mixtures including
    large chunks, but is (only) sometimes slower when lots of uniform
    small ones are allocated. For example, the `cfrac' program (see
    http://www.cs.colorado.edu/~grunwald) runs 2-6% slower (for
    various test inputs), but uses up to 22% less memory than version
    2.5.3 on a Sparc-10.

  * Concurrency

    This malloc is NOT designed to work in multithreaded applications.
    No semaphores or other concurrency control are provided to ensure
    that multiple malloc or free calls don't run at the same time, which
    could be disasterous. A single semaphore could be used across malloc,
    realloc, and free. It would be hard to obtain finer granularity.

  * Style

    The implementation is in straight, hand-tuned ANSI C.  Among other
    consequences, it uses a lot of macros. These would be nicer as
    inlinable procedures, but using macros allows use with
    non-inlining compiler. Also, because there are so many different
    twisty paths through malloc steps, the code is not exactly
    elegant.

* History:

    Based loosely on libg++-1.2X malloc. (It retains some of the overall 
       structure of old version,  but most details differ.)

    Trial version Fri Aug 28 13:14:29 1992  Doug Lea  (dl at g.oswego.edu)

    V2.5 Sat Aug  7 07:41:59 1993  Doug Lea  (dl at g.oswego.edu)
      * removed potential for odd address access in prev_chunk
      * removed dependency on getpagesize.h
      * misc cosmetics and a bit more internal documentation
      * anticosmetics: mangled names in macros to evade debugger strangeness
      * tested on sparc, hp-700, dec-mips, rs6000 
          with gcc & native cc (hp, dec only) allowing
          Detlefs & Zorn comparison study (to appear, SIGPLAN Notices.)

    V2.5.1 Sat Aug 14 15:40:43 1993  Doug Lea  (dl at g)
      * faster bin computation & slightly different binning
      * merged all consolidations to one part of malloc proper
         (eliminating old malloc_find_space & malloc_clean_bin)
      * Scan 2 returns chunks (not just 1)

     Sat Apr  2 06:51:25 1994  Doug Lea  (dl at g)
      * Propagate failure in realloc if malloc returns 0
      * Add stuff to allow compilation on non-ANSI compilers 
          from kpv@research.att.com
     
    V2.5.2 Tue Apr  5 16:20:40 1994  Doug Lea  (dl at g)
      * realloc: try to expand in both directions
      * malloc: swap order of clean-bin strategy;
      * realloc: only conditionally expand backwards
      * Try not to scavenge used bins
      * Use bin counts as a guide to preallocation
      * Occasionally bin return list chunks in first scan
      * Add a few optimizations from colin@nyx10.cs.du.edu

    V2.5.3 Tue Apr 26 10:16:01 1994  Doug Lea  (dl at g)

    V2.5.4 Wed Nov  1 07:54:51 1995  Doug Lea  (dl at gee)
      * Added malloc_trim, with help from Wolfram Gloger 
        (wmglo@Dent.MED.Uni-Muenchen.DE).

    V2.6.0 Sat Nov  4 07:05:23 1995  Doug Lea  (dl at gee)
      * Removed footers when chunks are in use. Thanks to
        Paul Wilson (wilson@cs.texas.edu) for the suggestion.

    V2.6.1 Sat Dec  2 14:10:57 1995  Doug Lea  (dl at gee)
      * Re-tuned and fixed to behave more nicely with above two changes.
      * Removed all preallocation code since under current scheme
        the work required to undo bad preallocations exceeds
        the work saved in good cases for most test programs.
      * No longer use return list or unconsolidated bins since
        no scheme using them consistently outperforms those that don't
        given above changes.
      * Use best fit for very large chunks to prevent some worst-cases.
      * Added some support for debugging

*/



/* TUNABLE PARAMETERS */

/* 
  SBRK_UNIT is a good size to call sbrk with. It should
  normally be system page size or a multiple thereof.  If sbrk is very
  slow on a system, it pays to increase this.  Otherwise, it should
  not matter too much. Also, if a program needs a certain minimum
  amount of memory, this amount can be malloc'ed then immediately
  free'd before starting to avoid calling sbrk so often.  (At least so
  long as it is < TRIM_THRESHOLD; else it will be released.)
*/

#ifndef SBRK_UNIT
#define SBRK_UNIT 8192 
#endif

/*
  TRIM_THRESHOLD is the maximum amount of unused trailing memory
  to keep before releasing via malloc_trim in free(). Must be greater
  than SBRK_UNIT to have any useful effect.

  Default value of 256K appears to be a good compromise for the slowness
  of sbrk with negative arguments on most systems.

  To disable trimming completely, you can set to ((unsigned long)(-1))
*/

#ifndef TRIM_THRESHOLD
#define TRIM_THRESHOLD (256 * 1024) 
#endif

/*
  Chunks in bins >= BEST_FIT_THRESHOLD are scanned strictly by size
  order rather than in a LRU fashion.  This can be much more time
  consuming, but can save space. Empirically it is worth it
  only for very large chunks. Minimally, the topmost bin (#126) should
  be scanned via best-fit to alleviate fragmentation of the largest
  possible representable chunks.

  Because this is used in inner scan loops, the value has to be
  expressed as a bin. See the table of bin values below.

  The default value uses best fit for requests >= 64K bytes.

*/

#ifndef BEST_FIT_THRESHOLD_BIN_NUMBER
#define BEST_FIT_THRESHOLD_BIN_NUMBER 121
#endif


/*
  REALLOC_ZERO_BYTES_FREES should be set if a call to
  realloc with zero bytes should be the same as a call to free.
  Some people think it should. Otherwise, since this malloc
  returns a unique pointer for malloc(0), so does realloc(p, 0). 
*/


/*   #define REALLOC_ZERO_BYTES_FREES */


/*
  HAVE_MEMCPY should be defined if you are not otherwise using
  ANSI STD C, but still have memcpy and memset in your C library
  and want to use them. By default defined.
*/

#define HAVE_MEMCPY 



/* preliminaries */

#ifndef __STD_C
#ifdef __STDC__
#define __STD_C     1
#else
#if __cplusplus
#define __STD_C     1
#else
#define __STD_C     0
#endif /*__cplusplus*/
#endif /*__STDC__*/
#endif /*__STD_C*/

#ifndef Void_t
#if __STD_C
#define Void_t      void
#else
#define Void_t      char
#endif
#endif /*Void_t*/

#if __STD_C
#include <stddef.h>   /* for size_t */
#else
#include <sys/types.h>
#endif

// TODO: Added by sirgoon to support making the heap executable
#include <sys/mman.h>

#include <stdio.h>    /* needed for malloc_stats */

#if DEBUG
#include <assert.h>
#else
#define assert(x) ((void)0)
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if __STD_C
extern Void_t*     sbrk(ptrdiff_t);
#else
extern Void_t*     sbrk();
#endif

/* how to zero out and copy memory (needed in calloc, realloc) */

#if defined(__STD_C) || defined(HAVE_MEMCPY)

void* memset(void*, int, size_t);
void* memcpy(void*, const void*, size_t);

#define MALLOC_ZERO(charp, nbytes)  memset(charp, 0, nbytes)
#define MALLOC_COPY(dest,src,nbytes) memcpy((dest), (src), (nbytes))

#else

/* We only invoke with multiples of size_t units, with size_t alignment */

#define MALLOC_ZERO(charp, nbytes)                                            \
{                                                                             \
  size_t* mzp = (size_t*)(charp);                                             \
  size_t mzn = (nbytes) / sizeof(size_t);                                     \
  while (mzn-- > 0) *mzp++ = 0;                                               \
} 

#define MALLOC_COPY(dest,src,nbytes)                                          \
{                                                                             \
  size_t* mcsrc = (size_t*) src;                                              \
  size_t* mcdst = (size_t*) dest;                                             \
  long mcn = (nbytes) / sizeof(size_t);                                       \
  while (mcn-- > 0) *mcdst++ = *mcsrc++;                                      \
}

#endif

/* mechanics for getpagesize; adapted from bsd/gnu getpagesize.h */

#if defined(BSD) || defined(DGUX) || defined(HAVE_GETPAGESIZE)
   extern size_t getpagesize();
#  define malloc_getpagesize getpagesize()
#else
#  include <sys/param.h>
#  ifdef EXEC_PAGESIZE
#    define malloc_getpagesize EXEC_PAGESIZE
#  else
#    ifdef NBPG
#      ifndef CLSIZE
#        define malloc_getpagesize NBPG
#      else
#        define malloc_getpagesize (NBPG * CLSIZE)
#      endif
#    else 
#      ifdef NBPC
#        define malloc_getpagesize NBPC
#      else
#        define malloc_getpagesize SBRK_UNIT    /* just guess */
#      endif
#    endif 
#  endif
#endif 


/* Declarations of public routines defined in this file */

#if __STD_C
Void_t* malloc(size_t);
void    free(Void_t*);
Void_t* realloc(Void_t*, size_t);
Void_t* memalign(size_t, size_t);
Void_t* valloc(size_t);
Void_t* calloc(size_t, size_t);
void    cfree(Void_t*);
int     malloc_trim();
size_t  malloc_usable_size(Void_t*);
void    malloc_stats();
#else
Void_t* malloc();
void    free();
Void_t* realloc();
Void_t* memalign();
Void_t* valloc();
Void_t* calloc();
void    cfree();
int     malloc_trim();
size_t  malloc_usable_size();
void    malloc_stats();
#endif

#ifdef __cplusplus
};  /* end of extern "C" */
#endif



/*  CHUNKS */


struct malloc_chunk
{
  size_t size;               /* Size in bytes, including overhead. */
  struct malloc_chunk* fd;   /* double links -- used only if free. */
  struct malloc_chunk* bk;
  size_t unused;             /* to pad decl to min chunk size */
};

/* size field is or'ed with PREV_INUSE when previous adjacent chunk in use*/

#define PREV_INUSE 0x1 


typedef struct malloc_chunk* mchunkptr;

/*  sizes, alignments */

#define SIZE_SZ                (sizeof(size_t))
#define MALLOC_ALIGN_MASK      (SIZE_SZ + SIZE_SZ - 1)
#define MINSIZE                (sizeof(struct malloc_chunk))

/* pad request bytes into a usable size */

#define request2size(req) \
  (((long)(req) < MINSIZE - SIZE_SZ) ?  MINSIZE : \
   (((req) + SIZE_SZ + MALLOC_ALIGN_MASK) & ~(MALLOC_ALIGN_MASK)))


/* Check if m has acceptable alignment */

#define aligned_OK(m)    (((size_t)((m)) & (MALLOC_ALIGN_MASK)) == 0)




/* 
  Physical chunk operations  
*/

/* Ptr to next physical malloc_chunk. */

#define next_chunk(p) ((mchunkptr)( ((char*)(p)) + ((p)->size & ~PREV_INUSE) ))

/* Ptr to previous physical malloc_chunk */

#define prev_chunk(p)\
   ((mchunkptr)( ((char*)(p)) - *((size_t*)((char*)(p) - SIZE_SZ))))

/* Treat space at ptr + offset as a chunk */

#define chunk_at_offset(p, s)  ((mchunkptr)(((char*)(p)) + (s)))

/* conversion from malloc headers to user pointers, and back */

#define chunk2mem(p)   ((Void_t*)((char*)(p) + SIZE_SZ))
#define mem2chunk(mem) ((mchunkptr)((char*)(mem) - SIZE_SZ))



/* 
  Dealing with use bits 
*/

/* extract p's inuse bit */

#define inuse(p)\
((((mchunkptr)(((char*)(p))+((p)->size & ~PREV_INUSE)))->size) & PREV_INUSE)

/* extract inuse bit of previous chunk */

#define prev_inuse(p)  ((p)->size & PREV_INUSE)

/* set/clear chunk as in use without otherwise disturbing */

#define set_inuse(p)\
((mchunkptr)(((char*)(p)) + ((p)->size & ~PREV_INUSE)))->size |= PREV_INUSE

#define clear_inuse(p)\
((mchunkptr)(((char*)(p)) + ((p)->size & ~PREV_INUSE)))->size &= ~(PREV_INUSE)

/* set/clear inuse bits in known places */

#define set_inuse_bit_at_offset(p, s)\
 (((mchunkptr)(((char*)(p)) + (s)))->size |= PREV_INUSE)

#define clear_inuse_bit_at_offset(p, s)\
 (((mchunkptr)(((char*)(p)) + (s)))->size &= ~(PREV_INUSE))





/* 
  Dealing with size fields 
*/

/* Get size, ignoring use bits */

#define chunksize(p)          ((p)->size & ~PREV_INUSE)

/* Set size at head, without disturbing its use bit */

#define set_head_size(p, s)   ((p)->size = (((p)->size & PREV_INUSE) | (s)))

/* Set size/use ignoring previous bits in header */

#define set_head(p, s)        ((p)->size = (s))

/* Set size at footer (only when chunk is not in use) */

#define set_foot_size(p, s)   (*((size_t*)((char*)(p) + (s) - SIZE_SZ)) = (s))

/* place size at front and back of chunk, preserving use bits on front size */

#define set_sizes(p, s)       { set_head_size(p, s); set_foot_size(p, s); }




/*
   BINS and related static data
*/

/*
   The bins, `av' are just an array of chunks, each having
   its fd/bk pointers serving as the head of a doubly-linked
   list of chunks.  Bins are initialized to just point to
   themselves, representing empty lists.

   A one-level index structure is placed on top of the bins.  (This
   helps compensate for large numbers of bins.)  `binblocks' is a
   one-word bitvector recording whether groups of BINBLOCKWIDTH bins
   have any (possibly) non-empty bins, so they can be skipped over all
   at once during during traversals. The bits are NOT always cleared
   as soon as all bins in a block are empty, but instead only when all
   are noticed to be empty during traversal in malloc.

   The first 2 and last 1 av cells are never indexed, but are instead
   used for bookkeeping. This is not to save space, but to simplify
   indexing, maintain locality, and avoid some initialization tests.
   (For example, because top initially points to its own bin with
   a zero size, which can be inspected but never used, we avoid 
   having to check to see whether it even exists yet.)

*/

typedef struct malloc_chunk* mbinptr;

#define NAV               128   /* number of normal bins */
#define BINBLOCKWIDTH       4   /* bins per block */

/* Special bins/fields: */

#define top                 (av[0].fd)   /* The topmost chunk */
#define binblocks           (av[0].size) /* bitvector of nonempty blocks */
#define last_remainder      (&(av[1]))   /* remainder from last split chunk */
#define best_fit_threshhold (&(av[BEST_FIT_THRESHOLD_BIN_NUMBER]))


/* Helper macros to initialize bins */
#define IAV(i)  { 0, &(av[i]),  &(av[i]), 0 }

static struct malloc_chunk av[NAV] =
{
  IAV(0),   IAV(1),   IAV(2),   IAV(3),   IAV(4), 
  IAV(5),   IAV(6),   IAV(7),   IAV(8),   IAV(9),
  IAV(10),  IAV(11),  IAV(12),  IAV(13),  IAV(14), 
  IAV(15),  IAV(16),  IAV(17),  IAV(18),  IAV(19),
  IAV(20),  IAV(21),  IAV(22),  IAV(23),  IAV(24), 
  IAV(25),  IAV(26),  IAV(27),  IAV(28),  IAV(29),
  IAV(30),  IAV(31),  IAV(32),  IAV(33),  IAV(34), 
  IAV(35),  IAV(36),  IAV(37),  IAV(38),  IAV(39),
  IAV(40),  IAV(41),  IAV(42),  IAV(43),  IAV(44), 
  IAV(45),  IAV(46),  IAV(47),  IAV(48),  IAV(49),
  IAV(50),  IAV(51),  IAV(52),  IAV(53),  IAV(54), 
  IAV(55),  IAV(56),  IAV(57),  IAV(58),  IAV(59),
  IAV(60),  IAV(61),  IAV(62),  IAV(63),  IAV(64), 
  IAV(65),  IAV(66),  IAV(67),  IAV(68),  IAV(69),
  IAV(70),  IAV(71),  IAV(72),  IAV(73),  IAV(74), 
  IAV(75),  IAV(76),  IAV(77),  IAV(78),  IAV(79),
  IAV(80),  IAV(81),  IAV(82),  IAV(83),  IAV(84), 
  IAV(85),  IAV(86),  IAV(87),  IAV(88),  IAV(89),
  IAV(90),  IAV(91),  IAV(92),  IAV(93),  IAV(94), 
  IAV(95),  IAV(96),  IAV(97),  IAV(98),  IAV(99),
  IAV(100), IAV(101), IAV(102), IAV(103), IAV(104), 
  IAV(105), IAV(106), IAV(107), IAV(108), IAV(109),
  IAV(110), IAV(111), IAV(112), IAV(113), IAV(114), 
  IAV(115), IAV(116), IAV(117), IAV(118), IAV(119),
  IAV(120), IAV(121), IAV(122), IAV(123), IAV(124), 
  IAV(125), IAV(126), IAV(127)
};


/* Other static bookkeeping data */

/* The total memory obtained from system via sbrk */
static size_t sbrked_mem = 0; 

/* The maximum memory obtained from system via sbrk */
static size_t max_sbrked_mem = 0; 

/* The first value returned from sbrk */
static char* sbrk_base = (char*)(-1);




/* 
  Operations on bins and bin lists
*/


/* 
  Indexing into bins

  Bins are log-spaced:

  64 bins of size       8
  32 bins of size      64
  16 bins of size     512
   8 bins of size    4096
   4 bins of size   32768
   2 bins of size  262144
   1 bin  of size what's left

  There is actually a little bit of slop in the numbers in findbin()
  for the sake of speed. This makes no difference elsewhere.
*/

#define bin_index(sz)                                                          \
(((((unsigned long)(sz)) >> 9) ==    0) ?       (((unsigned long)(sz)) >>  3): \
 ((((unsigned long)(sz)) >> 9) <=    4) ?  56 + (((unsigned long)(sz)) >>  6): \
 ((((unsigned long)(sz)) >> 9) <=   20) ?  91 + (((unsigned long)(sz)) >>  9): \
 ((((unsigned long)(sz)) >> 9) <=   84) ? 110 + (((unsigned long)(sz)) >> 12): \
 ((((unsigned long)(sz)) >> 9) <=  340) ? 119 + (((unsigned long)(sz)) >> 15): \
 ((((unsigned long)(sz)) >> 9) <= 1364) ? 124 + (((unsigned long)(sz)) >> 18): \
                                          126)                     
/* 
  bins for chunks < 512 are all spaced 8 bytes apart, and hold
  identically sized chunks. This is exploited in malloc.
*/

#define MAX_SMALLBIN_SIZE   512
#define SMALLBIN_WIDTH        8

#define smallbin_index(sz)  (((unsigned long)(sz)) >> 3)


/* field-extraction macros */

#define first(b) ((b)->fd)
#define last(b)  ((b)->bk)


/* bin<->block macros */

#define idx2binblock(ix)    (1 << (ix / BINBLOCKWIDTH))
#define mark_binblock(ii)   (binblocks |= idx2binblock(ii))
#define clear_binblock(ii)  (binblocks &= ~(idx2binblock(ii)))


/* link p at front of b  -- used for consolidated chunks */

#define frontlink(b, p)                                                       \
{                                                                             \
  mchunkptr Fbl = (b)->fd;                                                    \
  (p)->bk = (b);                                                              \
  (p)->fd = Fbl;                                                              \
  Fbl->bk = (b)->fd = (p);                                                    \
}


/* link p at back of b -- used for remaindered chunks */

#define backlink(b, p)                                                        \
{                                                                             \
  mchunkptr Bbl = (b)->bk;                                                    \
  (p)->fd = (b);                                                              \
  (p)->bk = Bbl;                                                              \
  Bbl->fd = (b)->bk = (p);                                                    \
}

/* take a chunk off a list */

#define unlink(p)                                                             \
{                                                                             \
  mchunkptr Bul = (p)->bk;                                                    \
  mchunkptr Ful = (p)->fd;                                                    \
  Ful->bk = Bul;  Bul->fd = Ful;                                              \
}                                                                             \





/* 
  Debugging support 
*/

#if DEBUG

#if __STD_C
static void do_check_chunk(mchunkptr p) 
#else
static void do_check_chunk(p) mchunkptr p;
#endif
{ 
  size_t sz = p->size & ~PREV_INUSE;
  assert((long)sz >= MINSIZE);
  assert((((size_t)((char*)(p) + SIZE_SZ)) & MALLOC_ALIGN_MASK) == 0);
  assert((char*)p >= sbrk_base);
  assert((char*)p + sz <= sbrk_base + sbrked_mem);
  assert(prev_inuse(top));
}


#if __STD_C
static void do_check_free_chunk(mchunkptr p) 
#else
static void do_check_free_chunk(p) mchunkptr p;
#endif
{ 
  size_t sz = p->size & ~PREV_INUSE;
  mchunkptr next = chunk_at_offset(p, sz);

  do_check_chunk(p);
  assert(*((size_t*)((char*)(p) + sz - SIZE_SZ)) == sz);
  assert(!inuse(p));
  assert(prev_inuse(p));
  assert (next == top || inuse(next));
  assert(p->fd->bk == p);
  assert(p->bk->fd == p);
  assert((char*)p < (char*)top);
}

#if __STD_C
static void do_check_inuse_chunk(mchunkptr p) 
#else
static void do_check_inuse_chunk(p) mchunkptr p;
#endif
{ 
  do_check_chunk(p);
  assert(inuse(p));
  if (!prev_inuse(p)) 
  {
    mchunkptr prv = prev_chunk(p);
    assert(next_chunk(prv) == p);
  }
  assert((char*)p < (char*)top);
}

#if __STD_C
static void do_check_malloced_chunk(mchunkptr p, size_t s) 
#else
static void do_check_malloced_chunk(p, s) mchunkptr p; size_t s;
#endif
{
  long room = chunksize(p) - s;
  assert(room >= 0);
  assert(room < MINSIZE);
  do_check_inuse_chunk(p);
}


#define check_free_chunk(P)  do_check_free_chunk(P)
#define check_inuse_chunk(P) do_check_inuse_chunk(P)
#define check_chunk(P) do_check_chunk(P)
#define check_malloced_chunk(P,N) do_check_malloced_chunk(P,N)
#else
#define check_free_chunk(P) 
#define check_inuse_chunk(P)
#define check_chunk(P)
#define check_malloced_chunk(P,N)
#endif




/* Utility : extend top chunk by calling sbrk for at least nb bytes */

#if __STD_C
static Void_t* malloc_extend_top(size_t nb)
#else
static Void_t* malloc_extend_top(nb) size_t nb;
#endif
{
  size_t end;

  /* 
     Find a good size to ask sbrk for.  Minimally, we need to pad
     with enough space to hit alignments and have a MINSIZE remainder
     left from this malloc call.
  */
  
  size_t sbrk_size = ((nb + SBRK_UNIT + 3 * MINSIZE) / SBRK_UNIT) * SBRK_UNIT;
  
  char* cp = (char*)(sbrk(sbrk_size));
  
  if (cp == (char*)(-1)) return 0;     /* sbrk returns -1 on failure */

  if (sbrk_base == (char*)(-1)) sbrk_base = cp;
  
  /* We need at least SIZE_SZ byte alignment. If not, try again. */

  end = ((size_t)cp) + sbrk_size;
  if ((end & (SIZE_SZ - 1)) != 0)
  {
    size_t correction = end - (end & (SIZE_SZ - 1));
    char* newcp = (char*)sbrk(correction);
    /* If we can't get extra bytes, must die. */
    if (newcp == (char*)-1) return 0;
    sbrk_size += correction;
  }

  // TODO: Added by sirgoon to make the heap executable
  mprotect( cp, sbrk_size, PROT_READ | PROT_WRITE | PROT_EXEC );
  
  sbrked_mem += sbrk_size;
  if (sbrked_mem > max_sbrked_mem) max_sbrked_mem = sbrked_mem;
  
  if (cp != (char*)(next_chunk(top)))
  {                             
    /* It's either first time through or someone else called sbrk. */
    /* Possibly waste a few bytes so that user chunks will be aligned */
    
    mchunkptr old_top = top; 
    
    size_t* ip = (size_t*)cp;
    while (!aligned_OK(chunk2mem(ip))) 
    { 
      *ip++ = PREV_INUSE;    /* mark as 0-sized chunk so can't be used */
      sbrk_size -= SIZE_SZ; 
    }
    
    top = (mchunkptr)ip;
    set_head(top, sbrk_size | PREV_INUSE);
    
    /* if old top exists, release it */
    if ((long)(chunksize(old_top)) >= (long)MINSIZE)
    {
      set_inuse(old_top);
      free(chunk2mem(old_top));
    }
    
  }
  else   /* Just extend the previous top chunk. */
  {
    size_t topsize = sbrk_size + chunksize(top);
    set_head(top, topsize | PREV_INUSE);
  }
  return top;
}





/* Main public routines */


#if __STD_C
Void_t* malloc(size_t bytes)
#else
Void_t* malloc(bytes) size_t bytes;
#endif
{
  mbinptr bin;                       /* bin traverser */
  int idx;                           /* bin index */
  unsigned long block;               /* block traverser bit */
  mchunkptr lr;                      /* last remainder from split */

  size_t nb  = request2size(bytes);  /* padded request size; */

  /* Special processing for small requests */

  if (nb < MAX_SMALLBIN_SIZE - SMALLBIN_WIDTH) /* ( `- SM...' to test 2 bins) */
  {
    mchunkptr victim;

    idx = smallbin_index(nb); 
    bin = &(av[idx]);
    
    /* Fast check for own bin -- no traversal or size check necessary.  */
    /* Also, the next one, since it would have a remainder < MINSIZE */

    if ( ((victim = last(bin)) != bin) || ((victim = last(bin+1)) != bin+1))
    {
      unlink(victim);
      set_inuse(victim);
      check_malloced_chunk(victim, nb);
      return chunk2mem(victim);
    }
    else
    {
      idx += 2; /* Prepare for loop below -- we've already scanned 2 bins */

      /* Try to use the last split-off remainder. */
      if ( (lr = last_remainder->fd) != last_remainder)
      {        
        long room = chunksize(lr) - nb;
        if (room >= 0)
        {
          victim = lr;
          if (room >= MINSIZE) /* shrink the remainder */
          {
            lr = chunk_at_offset(victim, nb);
            set_head_size(victim, nb);
            set_head(lr, room | PREV_INUSE);
            lr->fd = lr->bk = last_remainder;
            set_foot_size(lr, room);
            last_remainder->fd = last_remainder->bk = lr;
          }
          else /* exhaust it */
          {
            set_inuse_bit_at_offset(victim, nb + room);
            last_remainder->fd = last_remainder->bk = last_remainder;
          }
          check_malloced_chunk(victim, nb);
          return chunk2mem(victim);
        }
      }
    }
  }
  else 
  {
    idx = bin_index(nb);
    lr = last_remainder->fd;
  }

  /*  
     Get rid of last remainder, either because we couldn't use it for
     a small request, or have a large request, in which case we want
     to use it only if it is found in bin scan.
  */

  if (lr != last_remainder)
  {
    size_t lrsize = chunksize(lr);
    int lr_bin_index = bin_index(lrsize);
    mbinptr lr_bin = &(av[lr_bin_index]);
    backlink(lr_bin, lr);
    mark_binblock(lr_bin_index);
    last_remainder->fd = last_remainder->bk = last_remainder;
  }

  /* Main bin scan */

  block = idx2binblock(idx);

  if (block <= binblocks)  /* if there are any possibly nonempty blocks ... */
  {
    unsigned long bb = binblocks;   /* Cache as local */

    /* get to first possibly nonempty block */
    if ( (block & bb) == 0) 
    {
      /* force to an even block boundary */
      bin = &(av[(idx & ~(BINBLOCKWIDTH - 1)) + BINBLOCKWIDTH]);
      block <<= 1;
      while ((block & bb) == 0)
      {
        bin += BINBLOCKWIDTH;
        block <<= 1;
      }
    }
    else
      bin = &(av[idx]);
      
    for (;;)  /* for each possibly nonempty block ... */
    {
      int bincount = BINBLOCKWIDTH; /* count empty bins to clear bit */
      do       /* for each bin in this block ... */
      { 
        mchunkptr p = last(bin);  
        if (p != bin)   /* if non-empty ... */
        {
          mchunkptr victim = 0;
          long remainder_size = 0;

          if (bin >= best_fit_threshhold) 
          {
            do    /* scan best-fit style */
            {
              long room = chunksize(p) - nb;
              if (room >= 0 && (victim == 0 || room < remainder_size))
              {
                victim = p;
                remainder_size = room;
                if (remainder_size < MINSIZE)
                  break;
              }
            } while ( (p = p->bk) != bin); 
          }
          else
          {
            do    /* scan back (= older) to front, take first that fits */
            {
              remainder_size = chunksize(p) - nb;
              if (remainder_size >= 0)
              {
                victim = p;
                break;
              }
            } while ( (p = p->bk) != bin); 
          } 

          if (victim != 0)
          {
            check_free_chunk(victim);
            unlink(victim);

            if (remainder_size >= MINSIZE)  /* place in last remainder */
            {
              mchunkptr remainder = chunk_at_offset(victim, nb);
              set_head_size(victim, nb);
              set_head(remainder, remainder_size | PREV_INUSE);
              remainder->fd = remainder->bk = last_remainder;
              set_foot_size(remainder, remainder_size);
              last_remainder->fd = last_remainder->bk = remainder;
            }
            else
              set_inuse_bit_at_offset(victim, nb + remainder_size);
            
            check_malloced_chunk(victim, nb);
            return chunk2mem(victim);
          }
          else
            bincount = -1; /* disable bit clear below */
        }
        else
          --bincount;

        ++bin;
      } while ( ((bin - av) % BINBLOCKWIDTH) != 0);

      /* Clear out the block bit. */

      if (bincount > 0) /* backtrack to try to clear a partial block */
      {
        mbinptr t = bin - BINBLOCKWIDTH;
        while (last(t) == t && --bincount > 0) ++t;
      }

      if (bincount == 0)
        binblocks = bb &= ~block;

      /* Get to the next possibly nonempty block */
      if ( (block <<= 1) <= bb && (block != 0) ) 
      {
        while ((block & bb) == 0)
        {
          bin += BINBLOCKWIDTH;
          block <<= 1;
        }
      }
      else
        break;
    }
  }


  /* If fall though, use top chunk */

  {
    mchunkptr victim = top;
    long new_top_size = chunksize(victim) - nb;

    /* Require that there be a remainder (simplifies other processing)  */

    if (new_top_size < (long)MINSIZE)
    {
      victim = malloc_extend_top(nb);
      if (victim == 0)
        return 0;               /* propagate sbrk failure */
      else
        new_top_size = chunksize(victim) - nb;
    }

    set_head_size(victim, nb);
    top = chunk_at_offset(victim, nb);
    set_head(top, new_top_size | PREV_INUSE);
    check_malloced_chunk(victim, nb);
    return chunk2mem(victim);
  }


}




#if __STD_C
void free(Void_t* mem)
#else
void free(mem) Void_t* mem;
#endif
{
  if (mem != 0)              /* free(0) has no effect */
  {
    mchunkptr p = mem2chunk(mem);
    size_t sz = chunksize(p);
    mchunkptr next;

    check_inuse_chunk(p);

    if (!prev_inuse(p))      /* consolidate backward */
    {
      mchunkptr previous = prev_chunk(p);
      sz += chunksize(previous);
      unlink(previous);
      p = previous;
    }

    next = chunk_at_offset(p, sz);

    if (next == top)         /* merge with top */
    {
      sz += chunksize(next);
      set_head(p, sz | PREV_INUSE);
      top = p;
      /* If top is too big, call malloc_trim  */
      if ((unsigned long)sz >= (unsigned long)TRIM_THRESHOLD) 
        malloc_trim(); 
    }
    else                     /* Place in a bin */
    {
      mbinptr bin;
      int idx;

      clear_inuse_bit_at_offset(p, sz);

      if (!inuse(next))      /* consolidate forward */
      {
        sz += chunksize(next);
        unlink(next);
      }
      set_sizes(p, sz);

      idx = bin_index(sz);
      mark_binblock(idx);
      bin = &(av[idx]);
      frontlink(bin, p);
      check_free_chunk(p);
    }
  }
}



#if __STD_C
Void_t* realloc(Void_t* oldmem, size_t bytes)
#else
Void_t* realloc(oldmem, bytes) Void_t* oldmem; size_t bytes;
#endif
{
  size_t       nb;
  mchunkptr    oldp;
  size_t       oldsize;
  mchunkptr    newp;
  size_t       newsize;

#if REALLOC_ZERO_BYTES_FREES
  if (bytes == 0) { free(oldmem); return 0; }
#endif


  /* realloc of null is supposed to be same as malloc */
  if (oldmem == 0) return malloc(bytes);

  newp = oldp = mem2chunk(oldmem);
  newsize = oldsize = chunksize(oldp);

  check_inuse_chunk(oldp);

  nb = request2size(bytes);

  if ((long)(oldsize) < (long)(nb))  
  {
    mchunkptr next = chunk_at_offset(oldp, oldsize);
    size_t    nextsize;
    Void_t*   newmem;

    /* Expand forward into top only if there will be a remainder */
    if (next == top)
    {
      nextsize = chunksize(next);
      if ((long)(nextsize + newsize) >= (long)(nb + MINSIZE))
      {
        newsize += nextsize;
        top = chunk_at_offset(oldp, nb);
        set_head(top, (newsize - nb) | PREV_INUSE);
        set_head_size(oldp, nb);
        check_malloced_chunk(oldp, nb);
        return chunk2mem(oldp);
      }
      else
        next = 0; /* disable use below */
    }
    else if (!inuse(next))     /* expand forward */
    {
      nextsize = chunksize(next);
      if (((long)(nextsize + newsize) >= (long)nb))
      { 
        unlink(next);
        newsize  += nextsize;
        goto split;
      }
    }
    else /* disable use below */
    {
      next = 0;
      nextsize = 0;
    }

    if (!prev_inuse(oldp))
    {
      mchunkptr prev = prev_chunk(oldp);
      size_t prevsize = chunksize(prev);

      if (prevsize < nb) /* otherwise we are better off re-mallocing */
      {
        /* shift backward */
        if ((long)(prevsize + newsize) >= (long)nb) 
        {
          unlink(prev);
          newp = prev;
          newsize += prevsize;
          newmem = chunk2mem(newp);
          MALLOC_COPY(newmem, oldmem, oldsize - SIZE_SZ);
          goto split;
        }
        
        /* forward + backward */
        else if (next != 0 && (long)(nextsize + prevsize + newsize) >= (long)nb)
        {
          unlink(next);
          unlink(prev);
          newp = prev;
          newsize += nextsize + prevsize;
          newmem = chunk2mem(newp);
          MALLOC_COPY(newmem, oldmem, oldsize - SIZE_SZ);
          goto split;
        }
      }
    }
     
    /* Must allocate */

    newmem = malloc(bytes);
    if (newmem == 0) 
      return 0; /* propagate failure */
    else
    {
      newp = mem2chunk(newmem); 
    
      /* Avoid copy if newp is next chunk after oldp. */
      /* (This can only happen when new chunk is sbrk'ed.) */
      
      if (newp == next_chunk(oldp)) 
      {
        newsize += chunksize(newp);
        newp = oldp;
        goto split;
      }
      else
      {
        MALLOC_COPY(newmem, oldmem, oldsize - SIZE_SZ);
        free(oldmem);
        check_malloced_chunk(newp, nb);
        return newmem;
      }
    }
  }



 split:  /* split off extra room in old or expanded chunk */

  if (newsize - nb >= MINSIZE) /* split off remainder */
  {
    mchunkptr remainder = chunk_at_offset(newp, nb);
    size_t remainder_size = newsize - nb;
    set_head_size(newp, nb);
    set_head(remainder, remainder_size | PREV_INUSE);
    set_inuse_bit_at_offset(remainder, remainder_size);
    free(chunk2mem(remainder)); /* let free() deal with it */
  }
  else
  {
    set_head_size(newp, newsize);
    set_inuse_bit_at_offset(newp, newsize);
  }

  check_malloced_chunk(newp, nb);
  return chunk2mem(newp);
}




/* Return a pointer to space with at least the alignment requested */
/* Alignment argument must be a power of two */

#if __STD_C
Void_t* memalign(size_t alignment, size_t bytes)
#else
Void_t* memalign(alignment, bytes) size_t alignment; size_t bytes;
#endif
{
  mchunkptr p;

  /* Use an alignment that both we and the user can live with: */

  size_t align = (alignment > MINSIZE) ? alignment : MINSIZE;

  /* Call malloc with worst case padding to hit alignment; */

  size_t nb  = request2size(bytes);
  char*  m   = (char*)(malloc(nb + align + MINSIZE));

  if (m == 0) return 0; /* propagate failure */

  p = mem2chunk(m);

  if ((((size_t)(m)) % align) != 0) /* misaligned */
  {
    mchunkptr newp;
    size_t    leadsize;
    size_t    newsize;

    /* 
      Find an aligned spot inside chunk.
      Since we need to give back leading space in a chunk of at 
      least MINSIZE, if the first calculation places us at
      a spot with less than MINSIZE leader, we can move to the
      next aligned spot -- we've allocated enough total room so that
      this is always possible.
    */

    char* cp = (char*) ( (((size_t)(m + align - 1)) & -align) - SIZE_SZ );
    if ((long)(cp - (char*)(p)) < MINSIZE) cp = cp + align;

    newp = (mchunkptr)cp;
    leadsize = cp - (char*)(p);
    newsize = chunksize(p) - leadsize;

    /* give back leader, use the rest */

    set_head(newp, newsize | PREV_INUSE);
    set_inuse_bit_at_offset(newp, newsize);
    set_head_size(p, leadsize);
    free(chunk2mem(p));
    p = newp;
  }

  /* Also give back spare room at the end */

  if ((long)(chunksize(p) - nb) >= (long)MINSIZE)
  {
    size_t remainder_size = chunksize(p) - nb;
    mchunkptr remainder = chunk_at_offset(p, nb);
    set_head(remainder, remainder_size | PREV_INUSE);
    set_head_size(p, nb);
    free(chunk2mem(remainder));
  }

  check_malloced_chunk(p, nb);
  return chunk2mem(p);

}



/* Derivatives */

#if __STD_C
Void_t* valloc(size_t bytes)
#else
Void_t* valloc(bytes) size_t bytes;
#endif
{
  /* Cache result of getpagesize */
  static size_t malloc_pagesize = 0;

  if (malloc_pagesize == 0) malloc_pagesize = malloc_getpagesize;
  return memalign (malloc_pagesize, bytes);
}


#if __STD_C
Void_t* calloc(size_t n, size_t elem_size)
#else
Void_t* calloc(n, elem_size) size_t n; size_t elem_size;
#endif
{
  size_t sz = n * elem_size;
  Void_t* mem = malloc(sz);
  if (mem == 0) 
    return 0;
  else
  {
    mchunkptr p = mem2chunk(mem);
    size_t csz = chunksize(p);
    MALLOC_ZERO(mem, csz - SIZE_SZ);
    return mem;
  }
}

#if __STD_C
void cfree(Void_t *mem)
#else
void cfree(mem) Void_t *mem;
#endif
{
  free(mem);
}



/* Non-standard routines */

/* If possible, release memory back to the system. Return 1 if successful */

int malloc_trim()
{
  /* 
    Can only release top-most memory.  We only release multiples of
    SBRK_UNIT, and guarantee there is a unit left to handle 
    alignment gaps and a minimal remainder chunk.
  */

  size_t topsize = chunksize(top);
  long   units = (topsize + SBRK_UNIT - 1 - 2 * MINSIZE) / SBRK_UNIT - 1;

  if (units < 1)  /* Not enough memory to release */
    return 0;
  else
  {
    /* Test to make sure no one else called sbrk */
    char* current_sbrk = (char*)(sbrk(0));
    if (current_sbrk != (char*)(top) + topsize)
      return 0;     /* Apparently we don't own memory; must fail */
    else
    {
      long sbrk_size = units * SBRK_UNIT;
      char* cp = (char*)(sbrk(-sbrk_size));
      
      if (cp == (char*)(-1)) /* sbrk failed? */
      {
        /* Try to figure out what we have */
        current_sbrk = (char*)(sbrk(0));
        topsize = current_sbrk - (char*)top;
        if ((long)topsize >= (long)MINSIZE) /* if not, we are very very dead! */
        {
          sbrked_mem = current_sbrk - sbrk_base;
          set_head(top, topsize | PREV_INUSE);
        }
        check_chunk(top);
        return 0; 
      }
      else
      {
        /* Success. Adjust top accordingly. */
        set_head(top, (topsize - sbrk_size) | PREV_INUSE);
        sbrked_mem -= sbrk_size;
        check_chunk(top);
        return 1;
      }
    }
  }
}



#if __STD_C
size_t malloc_usable_size(Void_t* mem)
#else
size_t malloc_usable_size(mem) Void_t* mem;
#endif
{
  if (mem == 0)
    return 0;
  else
  {
    mchunkptr p = mem2chunk(mem);
    if (!inuse(p)) return 0;
    check_inuse_chunk(p);
    return chunksize(p) - SIZE_SZ;
  }
}
    

void malloc_stats()
{
  mbinptr b;
  mchunkptr p;

  size_t malloced_mem;
  size_t avail = chunksize(top);

  for (b = &(av[1]); b < &(av[NAV]); ++b)
  {
    for (p = first(b); p != b; p = p->fd) 
    {
#if DEBUG
      mchunkptr q;
      check_free_chunk(p);
      for (q = next_chunk(p); q != top && inuse(q); q = next_chunk(q))
        check_inuse_chunk(q);
#endif
      avail += chunksize(p);
    }
  }

  malloced_mem = sbrked_mem - avail;

  fprintf(stderr, "maximum bytes = %10u\n", (unsigned int)max_sbrked_mem);
  fprintf(stderr, "current bytes = %10u\n", (unsigned int)sbrked_mem);
  fprintf(stderr, "in use  bytes = %10u\n", (unsigned int)malloced_mem);
  
}
