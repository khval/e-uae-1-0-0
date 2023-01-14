
#include <stdlib.h>

#include <proto/exec.h>
#include <proto/dos.h>
# include <proto/intuition.h>
# include <proto/graphics.h>

#include "sysconfig.h"
#include "sysdeps.h"

#ifdef PICASSO96_SUPPORTED
#include "include/picasso96.h"
#endif

extern struct Screen *S;
extern struct Window *W;

#include "video_convert.h"

uint32 load32_p96_table[1 + (256 * 3)];		// 256 colors + 1 count

extern uint16 *vpal16;
extern uint32 *vpal32;

void init_lookup_15bit_to_16bit_le( void );
void init_lookup_15bit_to_16bit_be( void );

void init_lookup_16bit_to_32bit_le( void );
void init_lookup_16bit_to_32bit_be( void );

void set_vpal_8bit_to_16bit_le_2pixels(struct MyCLUTEntry *pal, uint32 num1);
void set_vpal_8bit_to_16bit_be_2pixels(struct MyCLUTEntry *pal, uint32 num1);
void set_vpal_8bit_to_32bit_le_2pixels(struct MyCLUTEntry *pal, uint32 num1);
void set_vpal_8bit_to_32bit_be_2pixels(struct MyCLUTEntry *pal, uint32 num1);

#define _LE_ARGB(n) 0xFF + (pal[n].Red << 8) +  (pal[n].Green << 16) + (pal[n].Blue << 24) 
#define _BE_ARGB(n) 0xFF000000 + (pal[n].Red << 16) +  (pal[n].Green << 8) + pal[n].Blue   

inline uint16 __pal_to_16bit(struct MyCLUTEntry *pal, int num)
{
	register unsigned int n;
	register unsigned int r;
	register unsigned int g;
	register unsigned int b;

	pal += num;
	r = pal -> Red & 0xF8;		// 4+1 = 5 bit
	g = pal -> Green  & 0xFC;	// 4+2 = 6 bit
	b = pal -> Blue  & 0xF8;	// 4+1 = 5 bit
	return ( (r << 8) | (g << 3) | (b >> 3));
}

void set_vpal_8bit_to_16bit_le_2pixels(struct MyCLUTEntry *pal, uint32 num1)
{
	int index;
	int num2;
	register unsigned int rgb;
	// Convert palette to 32 bits virtual buffer.

	// pixel [0..0],[0..255]

	for (num1=0;num1<256;num1++)
	{
		rgb = __pal_to_16bit(pal, num1);
		vpal32[num1] = ((rgb & 0xFF00) >> 8) | ((rgb & 0xFF) <<8);		// to LE
	}

	// pixel [0..255],[0..255]

	for (index=0;index<256*256;index++)
	{
		num2 = (index & 0x00FF);
		num1 = (index & 0xFF00) >> 8;

		vpal32[index] =  ((vpal32[num1] & 0xFFFF) << 16) | (vpal32[num2] & 0xFFFF) ;
	}
}

void set_vpal_8bit_to_16bit_be_2pixels(struct MyCLUTEntry *pal, uint32 num1)
{

	int index;
	int num2;
	register unsigned int rgb;
	// Convert palette to 32 bits virtual buffer.

	// pixel [0..0],[0..256]

	for (num1=0;num1<256;num1++)
	{
		vpal32[num1] = __pal_to_16bit(pal, num1);
	}

	// pixel [0,256],[0..256]		// because color 0 is not always black... (we need to redo the first 256 colors also)

	for (index=0;index<256*256;index++)
	{
		num2 = index & 255;
		num1 = (index >> 8) & 255;

		vpal32[index] =  vpal32[num1] << 16 | vpal32[num2];
	}
}

void set_vpal_8bit_to_32bit_le_2pixels(struct MyCLUTEntry *pal, uint32 num1)
{

	int index;
	int num2;
	register unsigned int rgb;
	// Convert palette to 32 bits virtual buffer.

	// pixel [0..0],[0..256]

	for (num1=0;num1<256;num1++)
	{
		vpal32[num1*2] = _LE_ARGB( num1 );
	}

	// pixel [0,256],[0..256]		// because color 0 is not always black... (we need to redo the first 256 colors also)

	for (index=0;index<256*256;index++)
	{
		num1 = (index >> 8) & 255;
		num2 = index & 255;

		vpal32[index*2] = vpal32[num1*2+1];
		vpal32[index*2+1] = vpal32[num2*2+1];
	}
}

void set_vpal_8bit_to_32bit_be_2pixels(struct MyCLUTEntry *pal, uint32 num1)
{
	int index;

	struct MyCLUTEntry *pal1;
	struct MyCLUTEntry *pal2;

	register unsigned int rgb;
	// Convert palette to 32 bits virtual buffer.

	// pixel [0,256],[0..256]		// because color 0 is not always black... (we need to redo the first 256 colors also)

	for (index=0;index<256*256;index++)
	{
		vpal32[index*2] =   _BE_ARGB( (index >> 8)  );  // ARGB
		vpal32[index*2+1] =  _BE_ARGB( (index & 0xFF) ) ;  // ARGB
	}
}

void set_vpal_8bit_to_32bit_be(struct MyCLUTEntry *pal, uint32 num1)
{
	vpal32[num1] =  _BE_ARGB( num1 ) ;  // ARGB
}

void SetPalette_8bit_screen (int start, int count)
{
	int i;

	load32_p96_table[ 0 ] = count << 16 | start;

	int offset = 1;

	for (i = start; i < start+count;  i++)
	{
		load32_p96_table[ offset ++ ] = 0x01010101 * picasso96_state.CLUT[i].Red;
		load32_p96_table[ offset ++ ] = 0x01010101 * picasso96_state.CLUT[i].Green;
		load32_p96_table[ offset ++ ] = 0x01010101 * picasso96_state.CLUT[i].Blue;
	}

	LoadRGB32( &(S -> ViewPort) , load32_p96_table );
}

void palette_8bit_update(struct MyCLUTEntry *pal, uint32 num)
{
	if (W -> BorderTop == 0)
	{
		SetPalette_8bit_screen(0, 256);
	}
}

void SetPalette_8bit_grayscreen (int start, int count)
{
	if (S == NULL) printf("no screen wtf\n");
	int i,r;
	load32_p96_table[ 0 ] = count << 16 | start;

	int offset = 1;

	for (i = start; i < start+count;  i++)
	{
		load32_p96_table[ offset ++ ] = 0x01010101 * i;
		load32_p96_table[ offset ++ ] = 0x01010101 * i;
		load32_p96_table[ offset ++  ] = 0x01010101 * i;
	}

	LoadRGB32( &(S -> ViewPort) , load32_p96_table );
}

void palette_8bit_gray_update(struct MyCLUTEntry *pal, uint32 num)
{

	if (W -> BorderTop == 0)
	{
		SetPalette_8bit_grayscreen(0, 256);
	}
}

void palette_8bit_nope(struct MyCLUTEntry *pal, uint32 num)
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
}

