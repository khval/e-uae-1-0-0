
/* This stuff should probably be moved elsewhere */
#if (defined __powerpc__ || defined __ppc__ || defined __POWERPC__ \
     || defined __PPC__) && defined __GNUC__
STATIC_INLINE void memcpy_bswap32 (void *dst, void *src, int n)
{
    int words = n / 4;

    if (words > 1) {
	uae_u32 tmp;
	asm volatile (
	    "addi    %2, %2, -1		\n\
	     mtctr   %2			\n\
	     lwz     %3, 0(%1)		\n\
	1:   stwbrx  %3, 0, %0		\n\
	     addi    %0, %0, 4  	\n\
	     lwzu    %3, 4(%1)		\n\
	     bdnz    1b         	\n\
	     stwbrx  %3, 0, %0"
	: "+r" (dst), "+r" (src), "+r" (words), "=r" (tmp)
	:
	:  "ctr", "memory");
   } else {
	uae_u32 tmp;
	asm volatile (
	    "lwz     %2, 0(%1)		\n\
	     stwbrx  %2, 0, %0"
	: "+r" (dst), "+r" (src), "=r" (tmp)
	:
	: "memory");
   }
}
#else
STATIC_INLINE void memcpy_bswap32 (void *dst, void *src, int n)
{
    int i = n / 4;
    uae_u32 *dstp = (uae_u32 *)dst;
    uae_u32 *srcp = (uae_u32 *)src;
    for ( ; i; i--)
	*dstp++ = bswap_32 (*srcp++);
}
#endif

