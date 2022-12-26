
#include <stdlib.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include "video_convert.h"

uint16 *lookup16bit = NULL;

extern uint16 *vpal16;
extern uint32 *vpal32;

struct video_convert_names vcn[] =	{

	{"convert_8bit_lookup_to_16bit",(void *) convert_8bit_lookup_to_16bit},
	{"convert_8bit_lookup_to_16bit_2pixels",(void *) convert_8bit_lookup_to_16bit_2pixels},
	{"convert_15bit_to_16bit_be",(void *) convert_15bit_to_16bit_be},
	{"convert_15bit_to_16bit_le",(void *) convert_15bit_to_16bit_le},
	{"convert_16bit_lookup_to_16bit",(void *) convert_16bit_lookup_to_16bit},
	{"convert_32bit_to_16bit_be",(void *) convert_32bit_to_16bit_be},
	{"convert_32bit_to_16bit_le",(void *) convert_32bit_to_16bit_le},
	{"convert_8bit_lookup_to_32bit_2pixels",(void *) convert_8bit_lookup_to_32bit_2pixels},
	{"convert_15bit_to_32bit",(void *) convert_15bit_to_32bit},
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
	int packs = pixels / 2;
	int n;

	for (n=0; n<packs;n++)
	{
		to[n] = vpal64[ from[n] ];
	}
}

void convert_15bit_to_16bit_le(  uint16 *from, uint16 *to,int  pixels )
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

void convert_15bit_to_16bit_be(  uint16 *from, uint16 *to,int  pixels )
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

void init_lookup_15bit_to_16bit_le(  )
{
	int n;
	register unsigned int rgb;
	register unsigned int rg;
	register unsigned int b;

	if (lookup16bit == NULL) lookup16bit = (uint16 *) malloc(65535 * sizeof(uint16));
	if (lookup16bit == NULL) return;

	for (n=0; n<65535;n++)
	{
		rg = (n & 0x007FE0) << 1;
		b = (n & 0x00001F) ;
		rgb =  (rg | b); 

		lookup16bit[n] = ((rgb & 0xFF00) >> 8)  | ((rgb & 0xFF) << 8);
	}
}

void init_lookup_15bit_to_16bit_be(  )
{
	int n;
	register unsigned int rgb;
	register unsigned int rg;
	register unsigned int b;

	if (lookup16bit == NULL) lookup16bit = (uint16 *) malloc(65535 * sizeof(uint16));
	if (lookup16bit == NULL) return;

	for (n=0; n<65535;n++)
	{
		rg = (n & 0x007FE0) << 1;
		b = (n & 0x00001F) ;
		lookup16bit[n] = (rg | b);
	}
}

void convert_16bit_lookup_to_16bit(  uint16 *from, uint16 *to,int  pixels )
{
	register int n;

	for (n=0; n<pixels;n++)
	{
		to[n] = lookup16bit[ from[n] ];
	}
}

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

void convert_15bit_to_32bit( uint16 *from, uint32 *to,int  pixels )
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


