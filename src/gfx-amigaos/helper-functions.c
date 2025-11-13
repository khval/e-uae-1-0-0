
#include <exec/types.h>
#include <proto/intuition.h>

#define NOBORDER FALSE

void gfx_center_window(struct Screen *My_Screen, BOOL BorderMode, ULONG window_width, ULONG window_height, ULONG *win_left, ULONG *win_top)
{
	struct DrawInfo *dri;
	ULONG bw, bh;
	bw =0; bh=0;

	if ( (dri = GetScreenDrawInfo(My_Screen) ) ) 
	{
		ULONG bw=0, bh=0;

		switch(BorderMode)
		{
			case NOBORDER:
				bw = 0;
				bh = 0;
				break;

#ifdef __amigaos4__
		default:
				bw = My_Screen->WBorLeft + My_Screen->WBorRight;
				bh = My_Screen->WBorTop + My_Screen->Font->ta_YSize + 1 + My_Screen->WBorBottom;
#endif
#ifdef __morphos__
			default:
				bw = GetSkinInfoAttrA(dri, SI_BorderLeft, NULL) + GetSkinInfoAttrA(dri, SI_BorderRight, NULL);
				bh = GetSkinInfoAttrA(dri, SI_BorderTopTitle, NULL) + GetSkinInfoAttrA(dri, SI_BorderBottom, NULL);
#endif
		}

		*win_left = (My_Screen->Width - (window_width + bw)) / 2;
		*win_top  = (My_Screen->Height - (window_height + bh)) / 2;

		FreeScreenDrawInfo(My_Screen, dri);
	}
}

