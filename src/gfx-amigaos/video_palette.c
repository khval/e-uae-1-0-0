
#include <stdlib.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include "sysconfig.h"
#include "sysdeps.h"

#ifdef PICASSO96_SUPPORTED
#include "picasso96.h"
#endif

#include "video_convert.h"

extern uint16 *vpal16;
extern uint32 *vpal32;


void init_lookup_15bit_to_16bit_le( void );
void init_lookup_15bit_to_16bit_be( void );
void set_vpal_8bit_to_16bit_le_2pixels(struct MyCLUTEntry *pal, uint32 num1);
void set_vpal_8bit_to_16bit_be_2pixels(struct MyCLUTEntry *pal, uint32 num1);
void set_vpal_8bit_to_32bit_le_2pixels(struct MyCLUTEntry *pal, uint32 num1);
void set_vpal_8bit_to_32bit_be_2pixels(struct MyCLUTEntry *pal, uint32 num1);


#define _LE_ARGB(n) 0xFF + (pal[n].Red << 8) +  (pal[n].Green << 16) + (pal[n].Blue << 24) 
#define _BE_ARGB(n) 0xFF000000 + (pal[n].Red << 16) +  (pal[n].green << 8) + pal[n].Blue   


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
//	int num2;

	struct MyCLUTEntry *pal1;
	struct MyCLUTEntry *pal2;

	register unsigned int rgb;
	// Convert palette to 32 bits virtual buffer.

	// pixel [0,256],[0..256]		// because color 0 is not always black... (we need to redo the first 256 colors also)

	for (index=0;index<256*256;index++)
	{
		pal1 = pal + ((index >> 8) & 255)*3;
		pal2 = pal + (index & 255)*3;

		vpal32[index*2] = 0xFF000000 + (pal1 -> Red << 16) +  (pal1 -> Green << 8) + pal1 -> Blue  ;  // ARGB
		vpal32[index*2+1] = 0xFF000000 + (pal2 -> Red << 16) +  (pal2 -> Green << 8) + pal2 -> Blue  ;  // ARGB
	}
}

