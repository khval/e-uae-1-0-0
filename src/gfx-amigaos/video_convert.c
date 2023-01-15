
#include <stdlib.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include "sysconfig.h"
#include "sysdeps.h"

#ifdef PICASSO96_SUPPORTED
#include "include/picasso96.h"
#endif

#include "video_convert.h"

extern struct TagItem tags_public[] ;

extern uint16 *vpal16;
extern uint32 *vpal32;

struct video_convert_names vcn[] =	{

	{"convert_8bit_lookup_to_16bit",(void *) convert_8bit_lookup_to_16bit},
	{"convert_8bit_lookup_to_16bit_2pixels",(void *) convert_8bit_lookup_to_16bit_2pixels},
	{"convert_15bit_to_16bit_be",(void *) convert_15bit_be_to_16bit_be},
	{"convert_15bit_to_16bit_le",(void *) convert_15bit_be_to_16bit_le},
	{"convert_16bit_lookup_to_16bit",(void *) convert_16bit_lookup_to_16bit},
	{"convert_32bit_to_16bit_be",(void *) convert_32bit_to_16bit_be},
	{"convert_32bit_to_16bit_le",(void *) convert_32bit_to_16bit_le},
	{"convert_8bit_lookup_to_32bit_2pixels",(void *) convert_8bit_lookup_to_32bit_2pixels},
//	{"convert_15bit_to_32bit",(void *) convert_15bit_to_32bit},
	{"convert_16bit_to_32bit",(void *) convert_16bit_to_32bit},

	{NULL,NULL}};


const char *get_name_converter_fn_ptr( void *fn_ptr)
{
	struct video_convert_names *i = vcn;
	while (i -> fn)
	{
		if (i -> fn == fn_ptr)	return i -> name;
		i ++;
	}
	return NULL;
}

void convert_8bit_to_16bit( char *from, uint16 *to,int  pixels )
{
	int n;
	register unsigned int rgb;
	register unsigned int r;
	register unsigned int g;
	register unsigned int b;

	for (n=0; n<pixels;n++)
	{
		to[n] = vpal16[from[n]];
	}
}

void convert_8bit_lookup_to_16bit(  char *from, uint16 *to,int  pixels )
{
	int n;

	for (n=0; n<pixels;n++)
	{
		to[n] = vpal16[ from[n] ];
	}
}

void convert_8bit_lookup_to_16bit_2pixels(  uint16 *from, uint32 *to,int  pixels )
{
	int packs = pixels / 2;
	int n;

	for (n=0; n<packs;n++)
	{
		to[n] = vpal32[ from[n] ];
	}
}

void convert_8bit_lookup_to_32bit_2pixels(  uint16 *from, double *to,int  pixels )
{
	double *vpal64 = (double *) vpal32;	// need to use doubles to get true 64bit registers.
	uint16 *end;
	int packs = pixels / 2;
	int n;

	end = from + packs;
	while (from<end)
	{
		*to++ = vpal64[ *from++ ];
		*to++ = vpal64[ *from++ ];
	}
}

void convert_15bit_be_to_16bit_le(  uint16 *from, uint16 *to,int  pixels )
{
	int n;
	register unsigned int rgb;
	register unsigned int rg;
	register unsigned int b;

	for (n=0; n<pixels;n++)
	{
		rgb = from[n];
		
		//        R          G         B
		// %11111 11111 00000

		rg = (rgb & 0x007FE0) << 1;
		b = (rgb & 0x00001F) ;
		rgb =  rg | b;

		to[n] = ((rgb & 0xFF00) >> 8)  | ((rgb & 0xFF) << 8);
	}
}

void convert_15bit_be_to_16bit_be(  uint16 *from, uint16 *to,int  pixels )
{
	int n;
	register unsigned int rgb;
	register unsigned int rg;
	register unsigned int b;

	for (n=0; n<pixels;n++)
	{
		rgb = from[n];
		rg = (rgb & 0x007FC0) << 1;
		b = (rgb & 0x00001F) ;
		to[n] =  rg | b;
	}
}

void convert_16bit_to_8bit( uint16 *from, char *to,int  pixels )
{
	register int n;

	if (!vpal16)
	{
		DebugPrintF("vpal16 is NULL, can't convert!!\n");
		return;
	}

	for (n=0; n<pixels;n++)
	{
		to[n] = (char) vpal16[ from[n] ];
	}
}

void convert_16bit_lookup_to_16bit(  uint16 *from, uint16 *to,int  pixels )
{
	register int n;

	if (!vpal16)
	{
		DebugPrintF("vpal16 is NULL, can't convert!!\n");
		return;
	}

	for (n=0; n<pixels;n++)
	{
		to[n] = vpal16[ from[n] ];
	}
}

void convert_16bit_to_32bit(  uint16 *from, uint32 *to,int  pixels )
{
	register int n;

	for (n=0; n<pixels;n++)
	{
		to[n] = vpal32[from[n]];
	}
}


/*
void convert_16bit_to_32bit( uint16 *from, uint32 *to,int  pixels )
{
	int n;
	register unsigned int rgb;
	register unsigned int r;
	register unsigned int g;
	register unsigned int b;

	for (n=0; n<pixels;n++)
	{
		rgb = from[n];
		r = (rgb & 0x00F800) << 8;
		g = (rgb & 0x0007E0) << 5;
		b = (rgb & 0x00001F) << 3;
		to[n] = 0xFF000000 | r | g | b;
	}
}
*/

void convert_32bit_to_16bit_le( uint32 *from, uint16 *to,int  pixels )
{
	int n;
	register unsigned int rgb;
	register unsigned int r;
	register unsigned int g;
	register unsigned int b;

	for (n=0; n<pixels;n++)
	{
		rgb = from[n];
		r = (rgb & 0xF80000) >> 8;
		g = (rgb & 0x00FC00) >> 5;
		b = (rgb & 0x0000F8) >> 3;
		rgb =  r | g | b;

		to[n] = ((rgb & 0xFF00) >> 8)  | ((rgb & 0xFF) << 8);
	}
}

void convert_32bit_to_16bit_be( uint32 *from, uint16 *to,int  pixels )
{
	int n;
	register unsigned int rgb;
	register unsigned int r;
	register unsigned int g;
	register unsigned int b;

	for (n=0; n<pixels;n++)
	{
		rgb = from[n];
		r = (rgb & 0xF80000) >> 8;
		g = (rgb & 0x00FC00) >> 5;
		b = (rgb & 0x0000F8) >> 3;
		to[n] =  r | g | b;
	}
}

void convert_8bit_to_32bit(  char *from, uint32 *to,int  pixels )
{
	int n;

	for (n=0; n<pixels;n++)
	{
		to[n] = vpal32[from[n]];
	}
}

void convert_32bit_swap(  char *from, char *to,int  pixels )
{
	int n;
	register uint32 a,b,c,d;

	for (n=0; n<pixels;n++)
	{
		a = *from++;
		b = *from++;
		c = *from++;
		*to++= *from++;
		*to++=c;
		*to++=b;
		*to++=a;
	}
}

void __convert_15bit_to_32bit( uint16 *from, uint32 *to,int  pixels )
{
	int n;
	register unsigned int rgb;
	register unsigned int r;
	register unsigned int g;
	register unsigned int b;

	for (n=0; n<pixels;n++)
	{
		rgb = from[n];
		r = (rgb & 0x007C00) << 9;
		g = (rgb & 0x0003E0) << 6;
		b = (rgb & 0x00001F) << 3;
		to[n] = 0xFF000000 | r | g | b;
	}
}

void init_lookup_16bit_swap(  )
{
	register unsigned int rgb;
	register unsigned int rg;
	register unsigned int b;

	if (vpal16 == NULL) vpal16 = (uint16 *) AllocVecTagList(0x10000 * sizeof(uint16), tags_public);
	if (vpal16 == NULL) return;

	for (rgb=0; rgb<0x10000;rgb++)
	{
		vpal16[rgb] = ((rgb & 0xFF00) >> 8)  | ((rgb & 0xFF) << 8);
	}
}

void init_lookup_15bit_be_to_8bit(  )
{
	int n;
	register unsigned int r,g,b;

	if (vpal16) FreeVec(vpal16); 
	vpal16 = (uint16 *) AllocVecTagList(0x10000 * sizeof(uint16), tags_public);
	if (vpal16 == NULL) return;

	for (n=0; n<0x10000;n++)
	{
		r = ((n >> 10) & 0x1F) * 255 / 0x1F;		// etch channel is 5 bits, two channels shifted out.
		g = ((n >> 5) & 0x1F) * 255 / 0x1F;
		b = (n & 0x1F) * 255 / 0x1F ;
		vpal16[n] = (r+g+b) /3;
	}
}

void init_lookup_16bit_be_to_8bit(  )
{
	int n;
	register unsigned int r,g,b;

	if (vpal16) FreeVec(vpal16); 
	vpal16 = (uint16 *) AllocVecTagList(0x10000 * sizeof(uint16), tags_public);
	if (vpal16 == NULL) return;

	for (n=0; n<0x10000;n++)
	{
		r = ((n >> 11) & 0x1F) * 255 / 0x1F;		// etch channel is 5 bits, two channels shifted out.
		g = ((n >> 5) & 0x3F) * 255 / 0x3F;
		b = (n & 0x1F) * 255 / 0x1F ;
		vpal16[n] = (r+g+b) /3;
	}
}

void init_lookup_15bit_be_to_16bit_le(  )
{
	int n;
	register unsigned int rgb;
	register unsigned int rg;
	register unsigned int b;

	if (vpal16) FreeVec(vpal16); 
	vpal16 = (uint16 *) AllocVecTagList(0x10000 * sizeof(uint16), tags_public);
	if (vpal16 == NULL) return;

	for (n=0; n<0x10000;n++)
	{
		rg = (n & 0x007FE0) << 1;
		b = (n & 0x00001F) ;
		rgb =  (rg | b); 

		vpal16[n] = ((rgb & 0xFF00) >> 8)  | ((rgb & 0xFF) << 8);
	}
}

void init_lookup_15bit_be_to_16bit_be(  )
{
	int n;
	register unsigned int rgb;
	register unsigned int rg;
	register unsigned int b;

	if (vpal16) FreeVec(vpal16); 
	vpal16 = (uint16 *) AllocVecTagList( 0x10000 * sizeof(uint16), tags_public);
	if (vpal16 == NULL) return;

	for (n=0; n<65535;n++)
	{
		rg = (n & 0x007FE0) << 1;
		b = (n & 0x00001F) ;
		vpal16[n] = (rg | b);
	}
}

void init_lookup_15bit_be_to_32bit_le( void  )
{
	int n;
	register unsigned int r;
	register unsigned int g;
	register unsigned int b;

	if (vpal32) FreeVec(vpal32); 
	vpal32 = AllocVecTagList(0x10000 * sizeof(uint32), tags_public);
	if (vpal32 == NULL) return;

	for (n=0; n<0x10000;n++)
	{
		r = ((n >> 10) & 0x1F) * 255 / 0x1F;		// etch channel is 5 bits, two channels shifted out.
		g = ((n >> 5) & 0x1F) * 255 / 0x1F;
		b = (n & 0x1F) * 255 / 0x1F ;
		vpal32[n] = b << 24 | g << 16 | r << 8 | 0xFF; 
	}
}

void init_lookup_15bit_be_to_32bit_be( void  )
{
	int n;
	register unsigned int r;
	register unsigned int g;
	register unsigned int b;

	if (vpal32) FreeVec(vpal32); 
	vpal32 = AllocVecTagList(0x10000 * sizeof(uint32), tags_public);
	if (vpal32 == NULL) return;

	for (n=0; n<0x10000;n++)
	{
		r = ((n >> 10) & 0x1F) * 255 / 0x1F;		// etch channel is 5 bits, two channels shifted out.
		g = ((n >> 5) & 0x1F) * 255 / 0x1F;
		b = (n & 0x1F) * 255 / 0x1F ;
		vpal32[n] = 0xFF000000 | r << 16 | g << 8 | b;
	}
}

void init_lookup_16bit_le_to_32bit_le( void )
{
	int n, rev;
	register unsigned int r;
	register unsigned int g;
	register unsigned int b;

	if (vpal32) FreeVec(vpal32); 
	vpal32 = AllocVecTagList( 0x10000 * sizeof(uint32), tags_public);
	if (vpal32 == NULL) return;

	for (n=0; n<0x10000;n++)
	{
		rev = (n << 8) | (n >> 8);
		r = ((rev >> 11) & 0x1F) * 255 / 0x1F;
		g = ((rev >> 5) & 0x3F) * 255 / 0x3F;
		b = (rev & 0x1F) * 255 / 0x1F ;
		vpal32[n] = b << 24 | g << 16 | r << 8 | 0xFF; 
	}
}

void init_lookup_16bit_be_to_32bit_le( void )
{
	int n;
	register unsigned int r;
	register unsigned int g;
	register unsigned int b;

	if (vpal32) FreeVec(vpal32); 
	vpal32 = AllocVecTagList( 0x10000 * sizeof(uint32), tags_public);
	if (vpal32 == NULL) return;

	for (n=0; n<0x10000;n++)
	{
		r = ((n >> 11) & 0x1F) * 255 / 0x1F;
		g = ((n >> 5) & 0x3F) * 255 / 0x3F;
		b = (n & 0x1F) * 255 / 0x1F ;
		vpal32[n] = b << 24 | g << 16 | r << 8 | 0xFF; 
	}
}

void init_lookup_16bit_le_to_32bit_be(  )
{
	int n, rev;
	register unsigned int r;
	register unsigned int g;
	register unsigned int b;

	if (vpal32 == NULL) vpal32 = AllocVecTagList( 0x10000 * sizeof(uint32), tags_public);
	if (vpal32 == NULL) return;

	for (n=0; n<0x10000;n++)
	{
		rev = (n << 8) | (n >> 8);
		r = ((rev >> 11) & 0x1F) * 255 / 0x1F;		// right shift green and blue
		g = ((rev >> 5) & 0x3F) * 255 / 0x3F;		// right shift blue.
		b = (rev & 0x1F) * 255 / 0x1F ;
		vpal32[n] = 0xFF000000 | r << 16 | g << 8 | b; 
	}
}

void init_lookup_16bit_be_to_32bit_be(  )
{
	int n;
	register unsigned int r;
	register unsigned int g;
	register unsigned int b;

	if (vpal32 == NULL) vpal32 = AllocVecTagList( 0x10000 * sizeof(uint32), tags_public);
	if (vpal32 == NULL) return;

	for (n=0; n<0x10000;n++)
	{
		r = ((n >> 11) & 0x1F) * 255 / 0x1F;		// right shift green and blue
		g = ((n >> 5) & 0x3F) * 255 / 0x3F;		// right shift blue.
		b = (n & 0x1F) * 255 / 0x1F ;
		vpal32[n] = 0xFF000000 | r << 16 | g << 8 | b; 
	}
}

void convert_32bit_to_8bit_grayscale(  char *from, char *to,int  pixels )
{
	int sum;
	int n;

	for (n=0; n<pixels;n++)
	{
		from ++;
		sum = *from++;
		sum += *from++;
		sum += *from++;
		*to++= sum / 3;
	}
}

