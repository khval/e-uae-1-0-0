
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <intuition/intuition.h>
#include <proto/graphics.h>
#include <proto/layers.h>

//#include "options.h"
//#include "custom.h"
//#include "gui.h"

#define ASM_SYM_FOR_FUNC(x) 

#define SIZEOF_SHORT 2
#define SIZEOF_INT 4
#define SIZEOF_VOID_P 4

#include "uae_types.h"
#include "xwin.h"

struct BackFillArgs
{
	int __place_holder__;
};

typedef struct CompositeHookData_s {
	struct BitMap *srcBitMap; // The source bitmap
	int32 srcWidth, srcHeight; // The source dimensions
	int32 offsetX, offsetY; // The offsets to the destination area relative to the window's origin
	int32 scaleX, scaleY; // The scale factors
	uint32 retCode; // The return code from CompositeTags()
} CompositeHookData;

static struct Rectangle rect;
static struct Hook hook;
static CompositeHookData hookData;

void BackFill_Func(struct RastPort *ArgRP, struct BackFillArgs *MyArgs);
void set_target_hookData( void );

extern struct Window    *W;

extern struct RastPort comp_RP;

struct Hook BackFill_Hook =
{
	{NULL, NULL},
	(HOOKFUNC) &BackFill_Func,
	NULL,
	NULL
};

static ULONG compositeHookFunc(
			struct Hook *hook, 
			struct RastPort *rastPort, 
			struct BackFillMessage *msg)
 {

	CompositeHookData *hookData = (CompositeHookData*)hook->h_Data;

	hookData->retCode = CompositeTags(
		COMPOSITE_Src, 
			hookData->srcBitMap, 
			rastPort->BitMap,
		COMPTAG_SrcWidth,   hookData->srcWidth,
		COMPTAG_SrcHeight,  hookData->srcHeight,
		COMPTAG_ScaleX, 	hookData->scaleX,
		COMPTAG_ScaleY, 	hookData->scaleY,
		COMPTAG_OffsetX,    msg->Bounds.MinX - (msg->OffsetX - hookData->offsetX),
		COMPTAG_OffsetY,    msg->Bounds.MinY - (msg->OffsetY - hookData->offsetY),
		COMPTAG_DestX,      msg->Bounds.MinX,
		COMPTAG_DestY,      msg->Bounds.MinY,
		COMPTAG_DestWidth,  msg->Bounds.MaxX - msg->Bounds.MinX + 1,
		COMPTAG_DestHeight, msg->Bounds.MaxY - msg->Bounds.MinY + 1,
//		COMPTAG_Flags,      COMPFLAG_SrcFilter | COMPFLAG_IgnoreDestAlpha | COMPFLAG_HardwareOnly,

		COMPTAG_SrcAlpha, COMP_FLOAT_TO_FIX(1.0f),
		COMPTAG_Flags,      COMPFLAG_SrcAlphaOverride | COMPFLAG_IgnoreDestAlpha | COMPFLAG_HardwareOnly,

		TAG_END);

	return 0;
}

void BackFill_Func(struct RastPort *ArgRP, struct BackFillArgs *MyArgs)
{
	if (W)
	{
		set_target_hookData();

		register struct RastPort *RPort = W->RPort;

		LockLayer(0, RPort->Layer);
		DoHookClipRects(&hook, RPort, &rect);
		UnlockLayer( RPort->Layer);
	}
}

void set_target_hookData( void )
{
 	rect.MinX = W->BorderLeft;
 	rect.MinY = W->BorderTop;
 	rect.MaxX = W->Width - W->BorderRight - 1;
 	rect.MaxY = W->Height - W->BorderBottom - 1;

 	float destWidth = rect.MaxX - rect.MinX + 1;
 	float destHeight = rect.MaxY - rect.MinY + 1;
 	float scaleX = (destWidth + 0.5f) / gfxvidinfo.width;
 	float scaleY = (destHeight + 0.5f) / gfxvidinfo.height;

	hookData.srcBitMap = comp_RP.BitMap;
	hookData.srcWidth = gfxvidinfo.width;
	hookData.srcHeight = gfxvidinfo.height;
	hookData.offsetX = W->BorderLeft;
	hookData.offsetY = W->BorderTop;
	hookData.scaleX = COMP_FLOAT_TO_FIX(scaleX);
	hookData.scaleY = COMP_FLOAT_TO_FIX(scaleY);
	hookData.retCode = COMPERR_Success;

	hook.h_Entry = (HOOKFUNC) compositeHookFunc;
	hook.h_Data = &hookData;

}



