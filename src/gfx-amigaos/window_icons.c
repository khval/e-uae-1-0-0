
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <intuition/intuition.h>
#include <intuition/imageclass.h>
#include <intuition/gadgetclass.h>

#include "window_icons.h"

struct Gadeget *add_window_button(struct Image *img, ULONG id, struct Window *win)
{
	struct Gadeget *retGad = NULL;

	if (img)
	{
		retGad = (struct Gadeget *) NewObject(NULL , "buttongclass", 
			GA_ID, id, 
			GA_RelVerify, TRUE, 
			GA_Image, img, 
			GA_TopBorder, TRUE, 
			GA_RelRight, 0, 
			GA_Titlebar, TRUE, 
			TAG_END);

		if (retGad)
		{
			AddGadget( win, (struct Gadget *) retGad, ~0 );
		}
	}

	return retGad;
}

void open_icon( struct Window *win, ULONG imageID, ULONG gadgetID, struct kIcon *icon  )
{
	struct DrawInfo *dri = GetScreenDrawInfo(win -> WScreen);

	icon -> image = (struct Image *) NewObject(NULL, "sysiclass", SYSIA_DrawInfo, dri, SYSIA_Which, imageID, TAG_END );
	if (icon -> image)
	{
		icon -> gadget = add_window_button( icon -> image, gadgetID,win);
	}
}

void dispose_icon(struct Window *win, struct kIcon *icon)
{
	if (icon -> gadget)
	{
		RemoveGadget( win, (struct Gadget *) icon -> gadget );
		icon -> gadget = NULL;
	}

	if (icon -> image)
	{
		DisposeObject( (Object *) icon -> image );
		icon -> image = NULL;
	}
}

