/*
  * UAE - The Un*x Amiga Emulator
  *
  * Amiga interface
  *
  * Copyright 1996,1997,1998 Samuel Devulder.
  * Copyright 2003-2007 Richard Drummond
  */

#include <stdlib.h>

#include "sysconfig.h"
#include "sysdeps.h"
#include "stdbool.h"

/****************************************************************************/

#include <proto/exec.h>
#include <proto/dos.h>

/*
#include <exec/execbase.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
*/

#include <graphics/gfxbase.h>
#include <graphics/displayinfo.h>

#include <libraries/asl.h>
#include <intuition/pointerclass.h>
#include <intuition/imageclass.h>

/****************************************************************************/

# ifdef __amigaos4__
#  define __USE_BASETYPE__
# endif
# include <proto/intuition.h>
# include <proto/graphics.h>
# include <proto/layers.h>
# include <proto/exec.h>
# include <proto/dos.h>
# include <proto/asl.h>
# include <workbench/workbench.h>
# include <proto/wb.h>
# include <proto/icon.h>

/****************************************************************************/

#include <ctype.h>
#include <signal.h>

/****************************************************************************/

#include "uae.h"
#include "options.h"
#include "custom.h"
#include "xwin.h"
#include "drawing.h"
#include "inputdevice.h"
#include "keyboard.h"
#include "keybuf.h"
#include "gui.h"
#include "debug.h"
#include "hotkeys.h"
#include "version.h"

#include "window_icons.h"
#include "video_convert.h"

#if 0
#define DEBUG_LOG(fmt,...) DebugPrintF(fmt, ##__VA_ARGS__)
#else
#define DEBUG_LOG(fmt,...) 
#endif

#ifdef PICASSO96_SUPPORTED
#include "picasso96.h"
#endif

#undef LockBitMapTags
#undef UnlockBitMap
extern struct GraphicsIFace *IGraphics;

#undef BitMap

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"

struct 
{
	int x,y;
	int bw,bh;
	int w,h
} save_window = {0,0,0,0,0,0} ;

#ifdef PICASSO96

int screen_is_picasso = 0;
static int screen_was_picasso;

static char *picasso_invalid_lines = NULL;

static int picasso_has_invalid_lines;
static int picasso_invalid_start, picasso_invalid_stop;
static int picasso_maxw = 0, picasso_maxh = 0;
static int mode_count;
extern struct picasso_vidbuf_description picasso_vidinfo;

static int bitdepth, bit_unit = 32;
static int current_width, current_height;

static int red_bits, green_bits, blue_bits;
static int red_shift, green_shift, blue_shift;

uint32 load32_p96_table[1 + (256 * 3)];		// 256 colors + 1 count

// this needs, to be changed when color is changed !!!!!

static int palette_update_start = 256;
static int palette_update_end   = 0;

void (*p96_conv_fn) (void *src, void *dest, int size) = NULL;

uint32 *vpal32 = NULL;
uint16 *vpal16 = NULL;
void (*set_palette_fn)(struct MyCLUTEntry *pal, uint32 num) = NULL;

void init_aga_comp( ULONG output_depth );

void alloc_picasso_invalid_lines( void );
void free_picasso_invalid_lines( void );

 void set_p96_func8( void );
 void set_p96_func16( void );
 void set_p96_func32( void );

 bool alloc_p96_draw_bitmap( int w, int h, int depth );

 void set_vpal_8bit_to_16bit_le_2pixels(struct MyCLUTEntry *pal, uint32 num);
 void set_vpal_8bit_to_16bit_be_2pixels(struct MyCLUTEntry *pal, uint32 num);

 void set_vpal_8bit_to_32bit_be(struct MyCLUTEntry *pal, uint32 num1);
 void set_vpal_8bit_to_32bit_be_2pixels(struct MyCLUTEntry *pal, uint32 num);
 void set_vpal_8bit_to_32bit_le_2pixels(struct MyCLUTEntry *pal, uint32 num);

struct screen_rect
{
	int w;
	int h;
};

ULONG DRAW_FMT_SRC = PIXF_A8R8G8B8;
ULONG COMP_FMT_SRC = PIXF_A8R8G8B8;


struct TagItem tags_any[] = {
							{AVT_Type,MEMF_SHARED},
							{AVT_Alignment,  16},
							{TAG_DONE,0}
						};

struct TagItem tags_public[] = {
							{AVT_Type,MEMF_SHARED},
							{AVT_Alignment,  16},
							{TAG_DONE,0}
						};

#endif

/****************************************************************************/

#define UAEIFF "UAEIFF"        /* env: var to trigger iff dump */
#define UAESM  "UAESM"         /* env: var for screen mode */

static void graphics_subshutdown (void);

static void open_window(void);
static void close_window(void);
static bool is_uniconifyed(void);
static bool enable_Iconify(void);
static void dispose_Iconify(void);

static int need_dither;        /* well.. guess :-) */
static int use_delta_buffer;   /* this will redraw only needed places */
static int output_is_true_color;            /* this is for cybergfx truecolor mode */
static int use_approx_color;

extern void write_log(const char *fmt, ... );

extern xcolnr xcolors[4096];

static uae_u8 *oldpixbuf;

/* Values for amiga_screen_type */
enum {
    UAESCREENTYPE_CUSTOM,
    UAESCREENTYPE_PUBLIC,
    UAESCREENTYPE_ASK,
    UAESCREENTYPE_LAST
};

/****************************************************************************/
/*
 * prototypes & global vars
 */

extern struct IntuitionBase    *IntuitionBase ;
extern struct GfxBase          *GfxBase ;
extern struct Library          *LayersBase ;
extern struct Library          *AslBase ;
extern struct Library          *CyberGfxBase ;
#ifdef USE_CGX_OVERLAY
extern struct Library          *CGXVideoBase;
#endif

extern struct AslIFace *IAsl;
extern struct GraphicsIFace *IGraphics;
extern struct LayersIFace *ILayers;
extern struct IntuitionIFace *IIntuition;
extern struct CyberGfxIFace *ICyberGfx;

bool empty_msg_queue(struct MsgPort *port);

unsigned long            frame_num; /* for arexx */

struct RastPort  comp_aga_RP;
struct RastPort  comp_p96_RP;
struct RastPort  conv_p96_RP ;
struct RastPort  *draw_p96_RP = &comp_p96_RP;	// draw direct to the output buffer.

static UBYTE			*Line = NULL;
static struct Screen		*S = NULL;;
//struct RastPort			*RP = NULL;
struct Window			*W = NULL;
static struct RastPort	*TempRPort = NULL;
static struct BitMap		*BitMap = NULL;

extern struct kIcon iconifyIcon;
extern struct kIcon padlockicon;
extern struct kIcon fullscreenicon;

static uae_u8 *CybBuffer = NULL;
static struct ColorMap  *CM = NULL;
static int              XOffset,YOffset;

static int os39;        /* kick 39 present */
static int usepub;      /* use public screen */
static int is_halfbrite;
static int is_ham;

static int   get_color_failed;
static int   maxpen;
static UBYTE pen[256];

#ifdef __amigaos4__
static int mouseGrabbed;
static int grabTicks;
#define GRAB_TIMEOUT 50
#endif

BOOL is_fullscreen_state = FALSE;

/*****************************************************************************/

static void set_title(void);
static LONG ObtainColor(ULONG, ULONG, ULONG);
static void ReleaseColors(void);

//static int  DoSizeWindow(struct Window *,int,int);

static int  init_ham(void);
static void ham_conv(UWORD *src, UBYTE *buf, UWORD len);
static int  RPDepth(struct RastPort *RP);
static int get_nearest_color(int r, int g, int b);

/****************************************************************************/

void main_window_led(int led, int on);
int do_inhibit_frame(int onoff);

extern void initpseudodevices(void);
extern void closepseudodevices(void);
extern void appw_init(struct Window *W);
extern void appw_exit(void);
extern void appw_events(void);

BOOL has_p96_mode( uae_u32 width, uae_u32 height, int depth, int max_modes );

void refresh_aga( void );
void update_gfxvidinfo_width_height( void );
void add_native_modes( int depth, int *count );
void p96_conv_all( void );
int init_comp_one( struct Window *W, ULONG output_depth, struct RastPort *rp, int w, int h );
void init_comp( struct Window *W );
static APTR setup_p96_buffer (struct vidbuf_description *gfxinfo);

extern int ievent_alive;

/***************************************************************************
 *
 * Default hotkeys
 *
 * We need a better way of doing this. ;-)
 */
static struct uae_hotkeyseq ami_hotkeys[] =
{
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_Q, -1,       INPUTEVENT_SPC_QUIT) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_R, -1,       INPUTEVENT_SPC_SOFTRESET) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_LSH, AK_R,   INPUTEVENT_SPC_HARDRESET) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_D, -1,       INPUTEVENT_SPC_ENTERDEBUGGER) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_S, -1,       INPUTEVENT_SPC_TOGGLEFULLSCREEN) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_G, -1,       INPUTEVENT_SPC_TOGGLEMOUSEGRAB) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_I, -1,       INPUTEVENT_SPC_INHIBITSCREEN) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_P, -1,       INPUTEVENT_SPC_SCREENSHOT) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_A, -1,       INPUTEVENT_SPC_SWITCHINTERPOL) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_NPADD, -1,   INPUTEVENT_SPC_INCRFRAMERATE) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_NPSUB, -1,   INPUTEVENT_SPC_DECRFRAMERATE) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_F1, -1,      INPUTEVENT_SPC_FLOPPY0) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_F2, -1,      INPUTEVENT_SPC_FLOPPY1) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_F3, -1,      INPUTEVENT_SPC_FLOPPY2) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_F4, -1,      INPUTEVENT_SPC_FLOPPY3) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_F5, -1,      INPUTEVENT_SPC_STATERESTOREDIALOG) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_LSH, AK_F1,  INPUTEVENT_SPC_EFLOPPY0) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_LSH, AK_F2,  INPUTEVENT_SPC_EFLOPPY1) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_LSH, AK_F3,  INPUTEVENT_SPC_EFLOPPY2) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_LSH, AK_F4,  INPUTEVENT_SPC_EFLOPPY3) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_LSH, AK_F5,  INPUTEVENT_SPC_STATESAVEDIALOG) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_NP0, -1,     INPUTEVENT_SPC_STATERESTORE) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_NP1, -1,     INPUTEVENT_SPC_STATERESTORE1) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_NP2, -1,     INPUTEVENT_SPC_STATERESTORE2) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_NP3, -1,     INPUTEVENT_SPC_STATERESTORE3) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_NP4, -1,     INPUTEVENT_SPC_STATERESTORE4) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_NP5, -1,     INPUTEVENT_SPC_STATERESTORE5) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_NP6, -1,     INPUTEVENT_SPC_STATERESTORE6) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_NP7, -1,     INPUTEVENT_SPC_STATERESTORE7) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_NP8, -1,     INPUTEVENT_SPC_STATERESTORE8) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_NP9, -1,     INPUTEVENT_SPC_STATERESTORE9) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_RSH, AK_NP0, INPUTEVENT_SPC_STATESAVE) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_RSH, AK_NP1, INPUTEVENT_SPC_STATESAVE1) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_RSH, AK_NP2, INPUTEVENT_SPC_STATESAVE2) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_RSH, AK_NP3, INPUTEVENT_SPC_STATESAVE3) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_RSH, AK_NP4, INPUTEVENT_SPC_STATESAVE4) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_RSH, AK_NP5, INPUTEVENT_SPC_STATESAVE5) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_RSH, AK_NP6, INPUTEVENT_SPC_STATESAVE6) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_RSH, AK_NP7, INPUTEVENT_SPC_STATESAVE7) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_RSH, AK_NP8, INPUTEVENT_SPC_STATESAVE8) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_RSH, AK_NP9, INPUTEVENT_SPC_STATESAVE9) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_F, -1,       INPUTEVENT_SPC_FREEZEBUTTON) },
    { HOTKEYS_END }
};

/****************************************************************************/

extern UBYTE cidx[4][8*4096];


/*
 * Dummy buffer locking methods
 */

static int dummy_lock (struct vidbuf_description *gfxinfo)
{
	return 1;
}

static void dummy_unlock (struct vidbuf_description *gfxinfo)
{
}

static void dummy_flush_screen (struct vidbuf_description *gfxinfo, int first_line, int last_line)
{
}

/*
 * Buffer methods for planar screens with no dithering.
 *
 * This uses a delta buffer to reduce the overhead of doing the chunky-to-planar
 * conversion required.
 */
STATIC_INLINE void flush_line_planar_nodither (struct vidbuf_description *gfxinfo, int line_no)
{
    int     xs      = 0;
    int     len     = gfxinfo->width;
    int     yoffset = line_no * gfxinfo->rowbytes;
    uae_u8 *src;
    uae_u8 *dst;
    uae_u8 *newp = gfxinfo->bufmem + yoffset;
    uae_u8 *oldp = oldpixbuf + yoffset;

    /* Find first pixel changed on this line */
    while (*newp++ == *oldp++) {
	if (!--len)
		return; /* line not changed - so don't draw it */
    }
    src   = --newp;
    dst   = --oldp;
    newp += len;
    oldp += len;

    /* Find last pixel changed on this line */
    while (*--newp == *--oldp)
	;

    len = 1 + (oldp - dst);
    xs  = src - (uae_u8 *)(gfxinfo->bufmem + yoffset);

    /* Copy changed pixels to delta buffer */
    CopyMem (src, dst, len);

    /* Blit changed pixels to the display */
    WritePixelLine8 (&comp_aga_RP, xs + XOffset, line_no + YOffset, len, dst, TempRPort);
}

static void flush_block_planar_nodither (struct vidbuf_description *gfxinfo, int first_line, int last_line)
{
    int line_no;

    for (line_no = first_line; line_no <= last_line; line_no++)
	flush_line_planar_nodither (gfxinfo, line_no);
}

/*
 * Buffer methods for planar screens with dithering.
 *
 * This uses a delta buffer to reduce the overhead of doing the chunky-to-planar
 * conversion required.
 */
STATIC_INLINE void flush_line_planar_dither (struct vidbuf_description *gfxinfo, int line_no)
{
    int      xs      = 0;
    int      len     = gfxinfo->width;
    int      yoffset = line_no * gfxinfo->rowbytes;
    uae_u16 *src;
    uae_u16 *dst;
    uae_u16 *newp = (uae_u16 *)(gfxinfo->bufmem + yoffset);
    uae_u16 *oldp = (uae_u16 *)(oldpixbuf + yoffset);

    /* Find first pixel changed on this line */
    while (*newp++ == *oldp++) {
	if (!--len)
		return; /* line not changed - so don't draw it */
    }
    src   = --newp;
    dst   = --oldp;
    newp += len;
    oldp += len;

    /* Find last pixel changed on this line */
    while (*--newp == *--oldp)
	;

    len = (1 + (oldp - dst));
    xs  = src - (uae_u16 *)(gfxinfo->bufmem + yoffset);

    /* Copy changed pixels to delta buffer */
    CopyMem (src, dst, len * 2);

    /* Dither changed pixels to Line buffer */
    DitherLine (Line, src, xs, line_no, (len + 3) & ~3, 8);

    /* Blit dithered pixels from Line buffer to the display */
    WritePixelLine8 ( &comp_aga_RP, xs + XOffset, line_no + YOffset, len, Line, TempRPort);
}

static void flush_block_planar_dither (struct vidbuf_description *gfxinfo, int first_line, int last_line)
{
    int line_no;

    for (line_no = first_line; line_no <= last_line; line_no++)
	flush_line_planar_dither (gfxinfo, line_no);
}

/*
 * Buffer methods for HAM screens.
 */
STATIC_INLINE void flush_line_ham (struct vidbuf_description *gfxinfo, int line_no)
{
    int     len = gfxinfo->width;
    uae_u8 *src = gfxinfo->bufmem + (line_no * gfxinfo->rowbytes);

    ham_conv ((void*) src, Line, len);
    WritePixelLine8 (&comp_aga_RP, 0, line_no, len, Line, TempRPort);

    return;
}

static void flush_block_ham (struct vidbuf_description *gfxinfo, int first_line, int last_line)
{
    int line_no;
    for (line_no = first_line; line_no <= last_line; line_no++) flush_line_ham (gfxinfo, line_no);
}


static void flush_line_cgx_v41 (struct vidbuf_description *gfxinfo, int line_no)
{
	if (comp_aga_RP.BitMap)
	{
		WritePixelArray (CybBuffer,
			 0 , line_no,
			 gfxinfo->rowbytes,
			COMP_FMT_SRC,
			 &comp_aga_RP,
			 XOffset,
			 YOffset + line_no,
			 gfxinfo->width,
			 1);
	}
}

static void flush_block_cgx_v41 (struct vidbuf_description *gfxinfo, int first_line, int last_line)
{
	if (comp_aga_RP.BitMap)
	{
		WritePixelArray (CybBuffer,
			0 , first_line,
			gfxinfo->rowbytes,
			COMP_FMT_SRC,
			&comp_aga_RP,
			XOffset,
			YOffset + first_line,
			gfxinfo->width,
			last_line - first_line + 1  );
	}
}


/****************************************************************************/

static void flush_clear_screen_gfxlib (struct vidbuf_description *gfxinfo)
{
	if (comp_aga_RP.BitMap)
	{
		if (output_is_true_color)
		{
			RectFillColor (&comp_aga_RP, W->BorderLeft, W->BorderTop,
				    W->Width - W->BorderLeft - W->BorderRight,
				    W->Height - W->BorderTop - W->BorderBottom,
				    0);
		}
		else
		{
			SetAPen  (&comp_aga_RP, get_nearest_color (0,0,0));
			RectFill (&comp_aga_RP, W->BorderLeft, W->BorderTop, W->Width - W->BorderRight, W->Height - W->BorderBottom);
		}
	}
	if (use_delta_buffer)  memset (oldpixbuf, 0, gfxinfo->rowbytes * gfxinfo->height);
}

/****************************************************************************/


static int RPDepth (struct RastPort *RP)
{
	if (output_is_true_color)
	{
		return GetBitMapAttr (RP->BitMap, (LONG)BMA_DEPTH);
	}
	else
		return RP->BitMap->Depth;
}

/****************************************************************************/

static int get_color (int r, int g, int b, xcolnr *cnp)
{
    int col;

    if (currprefs.amiga_use_grey)
	r = g = b = (77 * r + 151 * g + 29 * b) / 16;
    else {
	r *= 0x11;
	g *= 0x11;
	b *= 0x11;
    }

    r *= 0x01010101;
    g *= 0x01010101;
    b *= 0x01010101;
    col = ObtainColor (r, g, b);

    if (col == -1) {
	get_color_failed = 1;
	return 0;
    }

    *cnp = col;
    return 1;
}

/****************************************************************************/
/*
 * FIXME: find a better way to determine closeness of colors (closer to
 * human perception).
 */
static __inline__ void rgb2xyz (int r, int g, int b,	int *x, int *y, int *z)
{
    *x = r * 1024 - (g + b) * 512;
    *y = 886 * (g - b);
    *z = (r + g + b) * 341;
}

static __inline__ int calc_err (int r1, int g1, int b1, int r2, int g2, int b2)
{
    int x1, y1, z1, x2, y2, z2;

    rgb2xyz (r1, g1, b1, &x1, &y1, &z1);
    rgb2xyz (r2, g2, b2, &x2, &y2, &z2);
    x1 -= x2; y1 -= y2; z1 -= z2;
    return x1 * x1 + y1 * y1 + z1 * z1;
}

/****************************************************************************/

static int get_nearest_color (int r, int g, int b)
{
    int i, best, err, besterr;
    int colors;
    int br=0,bg=0,bb=0;

   if (currprefs.amiga_use_grey)
	r = g = b = (77 * r + 151 * g + 29 * b) / 256;

    best    = 0;
    besterr = calc_err (0, 0, 0, 15, 15, 15);
    colors  = is_halfbrite ? 32 :(1 << RPDepth (&comp_aga_RP));

    for (i = 0; i < colors; i++) {
	long rgb;
	int cr, cg, cb;

	rgb = GetRGB4 (CM, i);

	cr = (rgb >> 8) & 15;
	cg = (rgb >> 4) & 15;
	cb = (rgb >> 0) & 15;

	err = calc_err (r, g, b, cr, cg, cb);

	if(err < besterr) {
		best = i;
		besterr = err;
		br = cr; bg = cg; bb = cb;
	}

	if (is_halfbrite) {
		cr /= 2; cg /= 2; cb /= 2;
		err = calc_err (r, g, b, cr, cg, cb);
		if (err < besterr) {
		best = i + 32;
		besterr = err;
		br = cr; bg = cg; bb = cb;
		}
	}
    }
    return best;
}

/****************************************************************************/

static int init_true_colors_output (const struct RastPort *rp)
{
	int redbits,  greenbits,  bluebits;
	int redshift, greenshift, blueshift;
	int byte_swap = FALSE;
	int pixfmt;
	int found = TRUE;

	if (!rp)
	{
		printf("%s:%d: unexpected rp=NULL, function failed\n");
		return 0;
	}

	if (!rp->BitMap)
	{
		printf("%s:%d: unexpected rp->BitMap=NULL, function failed\n");
		return 0;
	}

	pixfmt = GetBitMapAttr (rp->BitMap, (LONG)BMA_PIXELFORMAT);

	switch (pixfmt)
	{
#ifdef WORDS_BIGENDIAN
		case PIXF_R5G5B5PC:
			byte_swap = TRUE;
		case PIXF_R5G5B5:
			redbits  = 5;  greenbits  = 5; bluebits  = 5;
			redshift = 10; greenshift = 5; blueshift = 0;
			break;
		case PIXF_R5G6B5PC:
			byte_swap = TRUE;
		case PIXF_R5G6B5:
			redbits  = 5;  greenbits  = 6;  bluebits  = 5;
			redshift = 11; greenshift = 5;  blueshift = 0;
			break;
		case PIXF_R8G8B8A8:
			redbits  = 8;  greenbits  = 8;  bluebits  = 8;
			redshift = 24; greenshift = 16; blueshift = 8;
			break;
		case PIXF_B8G8R8A8:
			redbits  = 8;  greenbits  = 8;  bluebits  = 8;
			redshift = 8;  greenshift = 16; blueshift = 24;
			break;
		case PIXF_A8R8G8B8:
			redbits  = 8;  greenbits  = 8;  bluebits  = 8;
			redshift = 16; greenshift = 8;  blueshift = 0;
			break;
#else
		case PIXF_R5G5B5:
			byte_swap = TRUE;
		case PIXF_R5G5B5PC:
			redbits  = 5;  greenbits  = 5;  bluebits  = 5;
			redshift = 10; greenshift = 0;  blueshift = 0;
			break;
		case PIXF_R5G6B5:
			byte_swap = TRUE;
		case PIXF_R5G6B5PC:
			redbits  = 5;  greenbits  = 6;  bluebits  = 5;
			redshift = 11; greenshift = 5;  blueshift = 0;
			break;
		case PIXF_B8G8R8A8:
			redbits  = 8;  greenbits  = 8;  bluebits  = 8;
			redshift = 16; greenshift = 8;  blueshift = 0;
			break;
		case PIXF_A8R8G8B8:
			redbits  = 8;  greenbits  = 8;  bluebits  = 8;
			redshift = 8;  greenshift = 16; blueshift = 24;
			break;
#endif
		default:
			redbits  = 0;  greenbits  = 0;  bluebits  = 0;
			redshift = 0;  greenshift = 0;  blueshift = 0;
			found = FALSE;
			break;
	}

	if (found)
	{
    		alloc_colors64k (redbits,  greenbits,  bluebits,
			 redshift, greenshift, blueshift,
			 0, 0, 0, byte_swap);

		write_log ("AMIGFX: Using a %d-bit true-colour display.\n", redbits + greenbits + bluebits);
	}
	else write_log ("AMIGFX: Unsupported pixel format.\n");

	return found;
}


static int init_colors (void)
{
	int success = TRUE;

	if (need_dither)
	{
		/* first try color allocation */
		int bitdepth = usepub ? 8 : RPDepth (W -> RPort);
		int maxcol;

		if (!currprefs.amiga_use_grey && bitdepth >= 3)

		do
		{
			get_color_failed = 0;
			setup_dither (bitdepth, get_color);

			if (get_color_failed) ReleaseColors ();
		} while (get_color_failed && --bitdepth >= 3);

		if( !currprefs.amiga_use_grey && bitdepth >= 3)
		{
			write_log ("AMIGFX: Color dithering with %d bits\n", bitdepth);
			return 1;
		}

		/* if that fail then try grey allocation */
		maxcol = 1 << (usepub ? 8 : RPDepth (&comp_aga_RP));

		do
		{
			get_color_failed = 0;
   			setup_greydither_maxcol (maxcol, get_color);
			if (get_color_failed) ReleaseColors ();
		} while (get_color_failed && --maxcol >= 2);

		// extra pass with approximated colors //
		if (get_color_failed)
		do
		{
			maxcol=2;
			use_approx_color = 1;
			get_color_failed = 0;
			setup_greydither_maxcol (maxcol, get_color);
			if (get_color_failed) ReleaseColors ();
		} while (get_color_failed && --maxcol >= 2);

		if (maxcol >= 2)
		{
			write_log ("AMIGFX: Grey dithering with %d shades.\n", maxcol);
				return 1;
		}

		return 0; /* everything failed :-( */
	}

	// No dither //
    switch (RPDepth (&comp_aga_RP)) {
	{
		case 6:
			if (is_halfbrite)
			{
				static int tab[]= {
				0x000, 0x00f, 0x0f0, 0x0ff, 0x08f, 0x0f8, 0xf00, 0xf0f,
				0x80f, 0xff0, 0xfff, 0x88f, 0x8f0, 0x8f8, 0x8ff, 0xf08,
				0xf80, 0xf88, 0xf8f, 0xff8, /* end of regular pattern */
				0xa00, 0x0a0, 0xaa0, 0x00a, 0xa0a, 0x0aa, 0xaaa,
				0xfaa, 0xf6a, 0xa80, 0x06a, 0x6af
			};

		int i;
		for (i = 0; i < 32; ++i)
			get_color (tab[i] >> 8, (tab[i] >> 4) & 15, tab[i] & 15, xcolors);
		for (i = 0; i < 4096; ++i) {
			uae_u32 val = get_nearest_color (i >> 8, (i >> 4) & 15, i & 15);
			xcolors[i] = val * 0x01010101;
		}
		write_log ("AMIGFX: Using 32 colours and half-brite\n");
		break;
		} else if (is_ham) {
		int i;
		for (i = 0; i < 16; ++i)
			get_color (i, i, i, xcolors);
		write_log ("AMIGFX: Using 12 bits pseudo-truecolor (HAM).\n");
		alloc_colors64k (4, 4, 4, 10, 5, 0, 0, 0, 0, 0);
		return init_ham ();
		}
		/* Fall through if !is_halfbrite && !is_ham */
	case 1: case 2: case 3: case 4: case 5: case 7: case 8:
		{
	    int maxcol = 1 << RPDepth (&comp_aga_RP);

			if (maxcol >= 8 && !currprefs.amiga_use_grey)
			{
				do
				{
					get_color_failed = 0;
					setup_maxcol (maxcol);
					alloc_colors256 (get_color);

					if (get_color_failed)	ReleaseColors ();

				} while (get_color_failed && --maxcol >= 8);
			}
			else
			{
				int i;
				for (i = 0; i < maxcol; ++i)
				{
					get_color ((i * 15)/(maxcol - 1), (i * 15)/(maxcol - 1), (i * 15)/(maxcol - 1), xcolors);
				}
			}

			write_log ("AMIGFX: Using %d colours.\n", maxcol);
			for (maxcol = 0; maxcol < 4096; ++maxcol)
			{
				int val = get_nearest_color (maxcol >> 8, (maxcol >> 4) & 15, maxcol & 15);
				xcolors[maxcol] = val * 0x01010101;
			}
			break;
		}

	case 15:
	case 16:
	case 24:
	case 32:

		success = init_true_colors_output ( (W -> RPort) );
		break;

    }
    return success;
}

/****************************************************************************/

static APTR blank_pointer;

/*
 * Initializes a pointer object containing a blank pointer image.
 * Used for hiding the mouse pointer
 */
static void init_pointer (void)
{
    static struct BitMap bitmap;
    static UWORD	 row[2] = {0, 0};

    InitBitMap (&bitmap, 2, 16, 1);
    bitmap.Planes[0] = (PLANEPTR) &row[0];
    bitmap.Planes[1] = (PLANEPTR) &row[1];

    blank_pointer = NewObject (NULL, POINTERCLASS,
				   POINTERA_BitMap,	(ULONG)&bitmap,
				   POINTERA_WordWidth,	1,
				   TAG_DONE);

    if (!blank_pointer)
	write_log ("Warning: Unable to allocate blank mouse pointer.\n");
}

/*
 * Free up blank pointer object
 */
static void free_pointer (void)
{
	if (blank_pointer)
	{
		DisposeObject (blank_pointer);
		blank_pointer = NULL;
	}
}

/*
 * Hide mouse pointer for window
 */
static void hide_pointer (struct Window *w)
{
	SetWindowPointer (w, WA_Pointer, (ULONG)blank_pointer, TAG_DONE);
}

/*
 * Restore default mouse pointer for window
 */
static void show_pointer (struct Window *w)
{
	SetWindowPointer (w, WA_Pointer, 0, TAG_DONE);
}

#ifdef __amigaos4__
/*
 * Grab mouse pointer under OS4.0. Needs to be called periodically
 * to maintain grabbed status.
 */
static void grab_pointer (struct Window *w)
{
	struct IBox box = {
		W->BorderLeft,
		W->BorderTop,
		W->Width  - W->BorderLeft - W->BorderRight,
		W->Height - W->BorderTop  - W->BorderBottom
	};

	SetWindowAttrs (W, WA_MouseLimits, &box, sizeof box);
	SetWindowAttrs (W, WA_GrabFocus, mouseGrabbed ? GRAB_TIMEOUT : 0, sizeof (ULONG));
}
#endif

/****************************************************************************/

typedef enum {
    DONT_KNOW = -1,
    INSIDE_WINDOW,
    OUTSIDE_WINDOW
} POINTER_STATE;

static POINTER_STATE pointer_state;

static POINTER_STATE get_pointer_state (const struct Window *w, int mousex, int mousey)
{
    POINTER_STATE new_state = OUTSIDE_WINDOW;

    /*
     * Is pointer within the bounds of the inner window?
     */
    if ((mousex >= w->BorderLeft)
     && (mousey >= w->BorderTop)
     && (mousex < (w->Width - w->BorderRight))
     && (mousey < (w->Height - w->BorderBottom))) {
	/*
	 * Yes. Now check whetehr the window is obscured by
	 * another window at the pointer position
	 */
	struct Screen *scr = w->WScreen;
	struct Layer  *layer;

	/* Find which layer the pointer is in */
	LockLayerInfo (&scr->LayerInfo);
	layer = WhichLayer (&scr->LayerInfo, scr->MouseX, scr->MouseY);
	UnlockLayerInfo (&scr->LayerInfo);

	/* Is this layer our window's layer? */
	if (layer == w->WLayer) {
		/*
		 * Yes. Therefore, pointer is inside the window.
		 */
		new_state = INSIDE_WINDOW;
	}
    }
    return new_state;
}

/****************************************************************************/

/*
 * Try to find a CGX/P96 screen mode which suits the requested size and depth
 */

static ULONG find_rtg_mode (ULONG *width, ULONG *height, ULONG depth)
{
	struct DisplayInfo dispi;
	struct DimensionInfo di;

	ULONG ID           = INVALID_ID;
	ULONG best_mode      = INVALID_ID;
	ULONG best_width     = (ULONG) -1L;
	ULONG best_height    = (ULONG) -1L;

	ULONG largest_mode   = INVALID_ID;
	ULONG largest_width  = 0;
	ULONG largest_height = 0;

	while ((ID = NextDisplayInfo (ID)) != (ULONG)INVALID_ID)
	{
		if (
			(GetDisplayInfoData( NULL, &di, sizeof(di) , DTAG_DIMS, ID)) &&
			(GetDisplayInfoData( NULL, &dispi, sizeof(dispi) ,  DTAG_DISP, ID))
		)
		{
			ULONG cwidth  = di.Nominal.MaxX -di.Nominal.MinX +1;
			ULONG cheight = di.Nominal.MaxY -di.Nominal.MinY +1;;
			ULONG cdepth  = (di.MaxDepth == 24 ?  32 : di.MaxDepth);

			if (cdepth == depth)
			{
				if (cheight >= largest_height && cwidth >= largest_width)
				{
					largest_mode = ID;
					largest_width = cwidth;
					largest_height = cheight;
				}

				if (cwidth >= *width && cheight >= *height)
				{
					if (cwidth <= best_width && cheight <= best_height)
					{
						best_width = cwidth;
						best_height = cheight;
						best_mode = ID;
					}
				}
			}
		}
	}

	if (best_mode != (ULONG)INVALID_ID)
	{
		*height = best_height;
		*width  = best_width;
	}
	else if (largest_mode != (ULONG)INVALID_ID)
	{
		best_mode = largest_mode;
		*height   = largest_height;
		*width    = largest_width;
	}

	return best_mode;
}

STATIC_INLINE int min(int a, int b)
{
	if (a<b)
		return a;
	else
		return b;
}

static int setup_customscreen (void)
{
	ULONG depth  = 0; // FIXME: Need to add some way of letting user specify preferred depth
	ULONG mode   = INVALID_ID;
	struct Screen *screen;
	ULONG error;
	ULONG width,height;

    static struct NewWindow NewWindowStructure = {
	0, 0, 800, 600, 0, 1,
	IDCMP_MOUSEBUTTONS | IDCMP_RAWKEY | IDCMP_DISKINSERTED | IDCMP_DISKREMOVED
		| IDCMP_ACTIVEWINDOW | IDCMP_INACTIVEWINDOW | IDCMP_MOUSEMOVE
		| IDCMP_DELTAMOVE,
	WFLG_SMART_REFRESH | WFLG_BACKDROP | WFLG_RMBTRAP | WFLG_NOCAREREFRESH
	 | WFLG_BORDERLESS | WFLG_ACTIVATE | WFLG_REPORTMOUSE,
	NULL, NULL, NULL, NULL, NULL, 5, 5, 800, 600,
	CUSTOMSCREEN
    };

	if (screen_is_picasso)
	{
		width  = picasso_vidinfo.width;
		height = picasso_vidinfo.height;
	}
	else
	{
		width  = gfxvidinfo.width;
		height = gfxvidinfo.height;
	}


    /* First try to find an RTG screen that matches the requested size  */
    {
	unsigned int i;
	const UBYTE preferred_depth[] = {15, 16, 32, 8}; /* Try depths in this order of preference */

	for (i = 0; i < sizeof preferred_depth && mode == (ULONG) INVALID_ID; i++) {
		depth = preferred_depth[i];
		mode = find_rtg_mode (&width, &height, depth);
	}
    }

    if (mode != (ULONG) INVALID_ID) {
	if (depth > 8)
		output_is_true_color = 1;
    } else {

	/* No (suitable) RTG screen available. Try a native mode */
	depth = os39 ? 8 : (currprefs.gfx_lores ? 5 : 4);
	mode = PAL_MONITOR_ID; // FIXME: should check whether to use PAL or NTSC.
	if (currprefs.gfx_lores)
		mode |= (gfxvidinfo.height > 256) ? LORESLACE_KEY : LORES_KEY;
	else
		mode |= (gfxvidinfo.height > 256) ? HIRESLACE_KEY : HIRES_KEY;
    }


	/* If the screen is larger than requested, centre UAE's display */
	if (width > (ULONG) gfxvidinfo.width)	XOffset = (width - gfxvidinfo.width) / 2;
	if (height > (ULONG) gfxvidinfo.height)	YOffset = (height - gfxvidinfo.height) / 2;

	do
	{
		screen = OpenScreenTags (NULL,
				SA_Width,     width,
				SA_Height,    height,
				SA_Depth,     depth,
				SA_DisplayID, mode,
				SA_Behind,    TRUE,
				SA_ShowTitle, FALSE,
				SA_Quiet,     TRUE,
				SA_ErrorCode, (ULONG)&error,
				TAG_DONE);

	} while (!screen && error == OSERR_TOODEEP && --depth > 1); /* Keep trying until we find a supported depth */

	if (!screen) 
	{
		/* TODO; Make this error report more useful based on the error code we got */
		write_log ("Error opening screen:%ld\n", error);
		gui_message ("Cannot open custom screen for UAE.\n");
		return 0;
	}

	S  = screen;
	CM = screen->ViewPort.ColorMap;

	NewWindowStructure.Width  = screen->Width;
	NewWindowStructure.Height = screen->Height;
	NewWindowStructure.Screen = screen;	   

	W = (void*)OpenWindow (&NewWindowStructure);
	if (!W)
	{
		write_log ("Cannot open UAE window on custom screen.\n");
		return 0;
	}

	hide_pointer (W);

	is_fullscreen_state = TRUE;

    return 1;
}

/****************************************************************************/

static void open_window_as_last(void)
{
	W = OpenWindowTags (NULL,
			WA_Title,        (ULONG)PACKAGE_NAME,
			WA_AutoAdjust,   TRUE,

			WA_Left, save_window.x,
			WA_Top, save_window.y,
			WA_InnerWidth, save_window.w - save_window.bw,
			WA_InnerHeight, save_window.h - save_window.bh,

			WA_PubScreen,    (ULONG)S,

			WA_IDCMP,        IDCMP_MOUSEBUTTONS | IDCMP_RAWKEY
					| IDCMP_ACTIVEWINDOW | IDCMP_INACTIVEWINDOW
					| IDCMP_MOUSEMOVE    | IDCMP_DELTAMOVE
					| IDCMP_CLOSEWINDOW  | IDCMP_REFRESHWINDOW
					| IDCMP_NEWSIZE | IDCMP_INTUITICKS | IDCMP_GADGETUP,

			WA_Flags,	 WFLG_DRAGBAR     | WFLG_DEPTHGADGET
					| WFLG_REPORTMOUSE | WFLG_RMBTRAP
					| WFLG_ACTIVATE    | WFLG_CLOSEGADGET
					| WFLG_SIZEGADGET | WFLG_SIZEBBOTTOM
					| WFLG_SMART_REFRESH,

			WA_MinWidth, gfxvidinfo.width + save_window.bw,
			WA_MinHeight, gfxvidinfo.height + save_window.bh,

			WA_MaxWidth, ~0,
			WA_MaxHeight, ~0,
			TAG_DONE);

	if (W)
	{
	 	open_icon( W, ICONIFYIMAGE, GID_ICONIFY, &iconifyIcon );
	 	open_icon( W, POPUPIMAGE, GID_FULLSCREEN, &fullscreenicon );
	 	open_icon( W, PADLOCKIMAGE, GID_PADLOCK, &padlockicon );
	}
}


static void open_window(void)
{
	W = OpenWindowTags (NULL,
			WA_Title,        (ULONG)PACKAGE_NAME,
			WA_AutoAdjust,   TRUE,
			WA_InnerWidth,   gfxvidinfo.width,
			WA_InnerHeight,  gfxvidinfo.height,
			WA_PubScreen,    (ULONG)S,

			WA_IDCMP,        IDCMP_MOUSEBUTTONS | IDCMP_RAWKEY
					| IDCMP_ACTIVEWINDOW | IDCMP_INACTIVEWINDOW
					| IDCMP_MOUSEMOVE    | IDCMP_DELTAMOVE
					| IDCMP_CLOSEWINDOW  | IDCMP_REFRESHWINDOW
					| IDCMP_NEWSIZE | IDCMP_INTUITICKS | IDCMP_GADGETUP,

			WA_Flags,	 WFLG_DRAGBAR     | WFLG_DEPTHGADGET
					| WFLG_REPORTMOUSE | WFLG_RMBTRAP
					| WFLG_ACTIVATE    | WFLG_CLOSEGADGET
					| WFLG_SIZEGADGET | WFLG_SIZEBBOTTOM
					| WFLG_SMART_REFRESH,

			WA_MaxWidth, ~0,
			WA_MaxHeight, ~0,
			TAG_DONE);

	if (W)
	{
	 	open_icon( W, ICONIFYIMAGE, GID_ICONIFY, &iconifyIcon );
	 	open_icon( W, POPUPIMAGE, GID_FULLSCREEN, &fullscreenicon );
	 	open_icon( W, PADLOCKIMAGE, GID_PADLOCK, &padlockicon );
	}
}

bool alloc_p96_draw_bitmap( int w, int h, int depth )
{
	InitRastPort(&conv_p96_RP);

	conv_p96_RP.BitMap = AllocBitMapTags( w, h, depth, 
			BMATags_PixelFormat, DRAW_FMT_SRC,
			BMATags_Displayable, FALSE,
			BMATags_Alignment, 4,
			TAG_END);

	if (conv_p96_RP.BitMap)
	{
		RectFillColor(&conv_p96_RP, 0, 0, w,h, 0xFFFF0000);
		return true;
	}

	return false;
}


int init_comp_one( struct Window *W, ULONG output_depth, struct RastPort *rp, int w, int h )
{
	InitRastPort(rp);

	rp -> BitMap = AllocBitMap( w,h, output_depth, BMF_DISPLAYABLE, W -> RPort -> BitMap);
	
	if (rp -> BitMap)
	{
		RectFillColor(rp, 0, 0, w,h, 0xFF00FF00);
	}
	else
	{
		printf("init_comp_one failed, no bitmap\n");
		printf("debug: w %d, h %d, depth %d\n",w,h,output_depth);
		return 0;
	}

	 output_is_true_color = (output_depth>8) ? 1 : 0;

	return 1;
}

extern void update_fullscreen_rect( int aspect );

struct vidbuf_description p96_buffer;

void set_p96_func8()
{
	DRAW_FMT_SRC = PIXF_CLUT;
	COMP_FMT_SRC = PIXF_NONE;	

	switch ( picasso_vidinfo.depth )
	{
		case 8: p96_conv_fn = NULL; break;
		case 15: p96_conv_fn = NULL; break;
		case 16: p96_conv_fn = NULL; break;
		case 32: p96_conv_fn = NULL; break;
	}
}

void set_p96_func16()
{
	switch ( picasso_vidinfo.depth )
	{
		case 8:	DRAW_FMT_SRC = PIXF_CLUT;
				COMP_FMT_SRC = PIXF_A8R8G8B8;	
				vpal32 = (uint32 *) AllocVecTagList ( 8 * 256 * 256 , tags_public  );	// 2 input pixel , 256 colors,  2 x 32bit output pixel. (0.5Mb)
//				init_lookup_8bit_to_16bit_be_2pixels();
				set_palette_fn = set_vpal_8bit_to_16bit_be_2pixels;
				p96_conv_fn = convert_8bit_lookup_to_16bit_2pixels; 
				break;

		case 15:	DRAW_FMT_SRC = PIXF_R5G6B5PC;
				COMP_FMT_SRC = PIXF_R5G6B5PC;
				init_lookup_15bit_to_16bit_le();
				p96_conv_fn = convert_15bit_to_16bit_be; break;

		case 16:	DRAW_FMT_SRC = PIXF_R5G6B5PC;
				COMP_FMT_SRC = PIXF_R5G6B5PC;
				p96_conv_fn = NULL; break;

		case 32:	DRAW_FMT_SRC = PIXF_A8R8G8B8;
				COMP_FMT_SRC = PIXF_R5G6B5PC;
				p96_conv_fn = convert_32bit_to_16bit_be; break;
	}
}

void set_p96_func32()
{
	switch ( picasso_vidinfo.depth )
	{
		case 8:	DRAW_FMT_SRC = PIXF_CLUT;
				COMP_FMT_SRC = PIXF_A8R8G8B8;
				vpal32 = (uint32 *) AllocVecTagList ( 8 * 256 * 256, tags_public  );	// 2 input pixel , 256 colors,  2 x 32bit output pixel. (0.5Mb)
//				init_lookup_8bit_to_32bit_be_2pixels();
				set_palette_fn = set_vpal_8bit_to_32bit_be_2pixels;
				p96_conv_fn = convert_8bit_lookup_to_32bit_2pixels; 
				break;

		case 15:	DRAW_FMT_SRC = PIXF_R5G5B5PC;
				COMP_FMT_SRC = PIXF_A8R8G8B8;	
				printf("%s:%d NYI... wtf!!\n",__FUNCTION__,__LINE__);
				break;

		case 16:	DRAW_FMT_SRC = PIXF_R5G6B5PC;
				COMP_FMT_SRC = PIXF_A8R8G8B8;	
				p96_conv_fn = convert_16bit_to_32bit ; 
				break;

		case 32:	COMP_FMT_SRC = PIXF_A8R8G8B8;	
				p96_conv_fn = NULL ; 
				break;
	}
}

void init_aga_comp( ULONG output_depth )
{
	switch ( output_depth )
	{
		case 8:	COMP_FMT_SRC = PIXF_CLUT; break;
		case 16:	COMP_FMT_SRC = PIXF_A8R8G8B8; break;
		case 24:
		case 32:	COMP_FMT_SRC = PIXF_A8R8G8B8; break;
	}

	init_comp_one( W,  output_depth, &comp_aga_RP, gfxvidinfo.width, gfxvidinfo.height  );
	update_fullscreen_rect( currprefs.gfx_correct_aspect );
}

void init_comp( struct Window *W )
{
	if (W == NULL)
	{
		printf("UNEXPECTED: window is not open!!!\n");
	}
	else
	{
		ULONG output_depth = GetBitMapAttr( W -> RPort -> BitMap, BMA_DEPTH );
	
		init_aga_comp(output_depth);
		draw_p96_RP = W -> RPort;

		if (screen_is_picasso) 
		{
			printf("output_depth: %d\n", output_depth);

			set_palette_fn = NULL;	// default.. no special palette function.
			p96_conv_fn = NULL;	// default.. nothing to convert

			switch ( output_depth )
			{
				case 8: set_p96_func8(); break;
				case 16: set_p96_func16(); break;
				case 24:
				case 32: set_p96_func32(); break;
			}

			if (output_depth != 8)
			{
				printf("**** this is a true color output *** \n");
				init_comp_one( W, output_depth, &comp_p96_RP, picasso_vidinfo.width, picasso_vidinfo.height );
				draw_p96_RP = &comp_p96_RP;
			}

			if (p96_conv_fn)
			{
				printf("**** Need to convert *** \n");
				if (alloc_p96_draw_bitmap( picasso_vidinfo.width, picasso_vidinfo.height, picasso_vidinfo.depth ))
				{
					draw_p96_RP = &conv_p96_RP;
				}
				else
				{
					printf("*** Failed to alloc p96 draw buffer ***\n");
					p96_conv_fn = NULL;
				}
			}
		}

		if (W->BorderTop == 0)
		{
			RectFillColor(W -> RPort, 
				0, 
				0, 
				W -> Width , 
				W -> Height,
				0xFF000000);
		}
	}

	if (screen_is_picasso)
	{
		printf("*** this is a picasso96 screen, (using picasso_vidinfo.width, picasso_vidinfo.height!)\n");
	}
	else
	{
		printf("*** this is a AGA screen, (using gfxvidinfo.width, gfxvidinfo.height)\n");
	}
}


static int setup_publicscreen(void)
{
	char *pubscreen = strlen (currprefs.amiga_publicscreen) ? currprefs.amiga_publicscreen : NULL;

	S = LockPubScreen (pubscreen);
	if (!S)
	{
		gui_message ("Cannot open UAE window on public screen '%s'\n",
			pubscreen ? pubscreen : "default");
			return 0;
	}

	CM = S->ViewPort.ColorMap;

	if ((S->ViewPort.Modes & (HIRES | LACE)) == HIRES)
	{
		if (gfxvidinfo.height + S->BarHeight + 1 >= S->Height)
		{
			gfxvidinfo.height >>= 1;
			currprefs.gfx_correct_aspect = 1;
		}
	}

	if (save_window.w)
	{
		open_window_as_last();
	}
	else
	{
		open_window();
	}

	UnlockPubScreen (NULL, S);

	if (!W)
	{
		write_log ("Can't open window on public screen!\n");
		CM = NULL;
		return 0;
	}

	XOffset = 0;
	YOffset = 0;

	is_fullscreen_state = FALSE;

	return 1;
}

/****************************************************************************/

static char *get_num (char *s, int *n)
{
   int i=0;
   while(isspace(*s)) ++s;
   if(*s=='0') {
     ++s;
     if(*s=='x' || *s=='X') {
       do {char c=*++s;
           if(c>='0' && c<='9') {i*=16; i+= c-'0';}    else
           if(c>='a' && c<='f') {i*=16; i+= c-'a'+10;} else
           if(c>='A' && c<='F') {i*=16; i+= c-'A'+10;} else break;
       } while(1);
     } else while(*s>='0' && *s<='7') {i*=8; i+= *s++ - '0';}
   } else {
     while(*s>='0' && *s<='9') {i*=10; i+= *s++ - '0';}
   }
   *n=i;
   while(isspace(*s)) ++s;
   return s;
}

/****************************************************************************/

static void get_displayid (ULONG *DI, LONG *DE)
{
   char *s;
   int di,de;

   *DI=INVALID_ID;
   s=getenv(UAESM);if(!s) return;
   s=get_num(s,&di);
   if(*s!=':') return;
   s=get_num(s+1,&de);
   if(!de) return;
   *DI=di; *DE=de;
}

/****************************************************************************/

static int setup_userscreen (void)
{
    struct ScreenModeRequester *ScreenRequest;
    ULONG DisplayID;

    LONG ScreenWidth = 0, ScreenHeight = 0, Depth = 0;
    UWORD OverscanType = OSCAN_STANDARD;
    BOOL AutoScroll = TRUE;
    int release_asl = 0;

	ScreenRequest = AllocAslRequest (ASL_ScreenModeRequest, NULL);

	if (!ScreenRequest)
	{
		write_log ("Unable to allocate screen mode requester.\n");
		return 0;
	}

    get_displayid (&DisplayID, &Depth);

    if (DisplayID == (ULONG)INVALID_ID) {
	if (AslRequestTags (ScreenRequest,
			ASLSM_TitleText, (ULONG)"Select screen display mode",
			ASLSM_InitialDisplayID,    0,
			ASLSM_InitialDisplayDepth, 8,
			ASLSM_InitialDisplayWidth, gfxvidinfo.width,
			ASLSM_InitialDisplayHeight,gfxvidinfo.height,
			ASLSM_MinWidth,            320, //currprefs.gfx_width_win,
			ASLSM_MinHeight,           200, //currprefs.gfx_height_win,
			ASLSM_DoWidth,             TRUE,
			ASLSM_DoHeight,            TRUE,
			ASLSM_DoDepth,             TRUE,
			ASLSM_DoOverscanType,      TRUE,
			ASLSM_PropertyFlags,       0,
			ASLSM_PropertyMask,        DIPF_IS_DUALPF | DIPF_IS_PF2PRI,
			TAG_DONE)) {
		ScreenWidth  = ScreenRequest->sm_DisplayWidth;
		ScreenHeight = ScreenRequest->sm_DisplayHeight;
		Depth        = ScreenRequest->sm_DisplayDepth;
		DisplayID    = ScreenRequest->sm_DisplayID;
		OverscanType = ScreenRequest->sm_OverscanType;
		AutoScroll   = ScreenRequest->sm_AutoScroll;
	} else
		DisplayID = INVALID_ID;
    }
    FreeAslRequest (ScreenRequest);

    if (DisplayID == (ULONG)INVALID_ID)
	return 0;

    output_is_true_color = (Depth > 8)  ? 1 : 0;

    if ((DisplayID & HAM_KEY) && !output_is_true_color ) Depth = 6; /* only ham6 for the moment */

#if 0
    if(DisplayID & DIPF_IS_HAM) Depth = 6; /* only ham6 for the moment */
#endif

	S = OpenScreenTags (NULL,
			SA_DisplayID,			DisplayID,
			SA_Width,				ScreenWidth,
			SA_Height,			ScreenHeight,
			SA_Depth,			Depth,
			SA_Overscan,			OverscanType,
			SA_AutoScroll,			AutoScroll,
			SA_ShowTitle,			FALSE,
			SA_Quiet,				TRUE,
			SA_Behind,			TRUE,
			SA_PubName,			(ULONG)"UAE",
			/* v39 stuff here: */
			(os39 ? SA_BackFill : TAG_DONE), (ULONG)LAYERS_NOBACKFILL,
			SA_SharePens,			TRUE,
			SA_Interleaved,			TRUE,
			SA_Exclusive,			FALSE,
			TAG_DONE);
	if (!S)
	{
		gui_message ("Unable to open the requested screen.\n");
		return 0;
	}

	CM           =  S->ViewPort.ColorMap;
	is_halfbrite = (S->ViewPort.Modes & EXTRA_HALFBRITE);
	is_ham       = (S->ViewPort.Modes & HAM);

	W = OpenWindowTags (NULL,
			WA_Width,		S -> Width,
			WA_Height,		S -> Height,
			WA_CustomScreen,	(ULONG)S,
			WA_Backdrop,		TRUE,
			WA_Borderless,	TRUE,
			WA_RMBTrap,		TRUE,
			WA_Activate,		TRUE,
			WA_ReportMouse,	TRUE,
			WA_IDCMP,		IDCMP_MOUSEBUTTONS
						 | IDCMP_RAWKEY
						 | IDCMP_DISKINSERTED
						 | IDCMP_DISKREMOVED
						 | IDCMP_ACTIVEWINDOW
						 | IDCMP_INACTIVEWINDOW
						 | IDCMP_MOUSEMOVE
						 | IDCMP_DELTAMOVE,
			(os39 ? WA_BackFill : TAG_IGNORE),   (ULONG) LAYERS_NOBACKFILL,
			TAG_DONE);

	if(!W)
	{
		write_log ("AMIGFX: Unable to open the window.\n");
		CloseScreen (S);
		S  = NULL;
		CM = NULL;
		return 0;
	}

	RectFillColor(W -> RPort, 
		0, 
		0, 
		W -> Width , 
		W -> Height,
		0xFF000000);

	hide_pointer (W);
	PubScreenStatus (S, 0);

	write_log ("AMIGFX: Using screenmode: 0x%lx: bits: %ld\n",DisplayID, Depth);

	is_fullscreen_state = TRUE;

	return 1;
}

/****************************************************************************/

int graphics_setup (void)
{
	if (((struct ExecBase *)SysBase)->LibNode.lib_Version < 36)
	{
		write_log ("UAE needs OS 2.0+ !\n");
		return 0;
	}

	os39 = (((struct ExecBase *)SysBase)->LibNode.lib_Version >= 39);

	init_pointer ();
	initpseudodevices ();

	atexit (graphics_leave);

	return 1;
}

/****************************************************************************/

static struct Window *saved_prWindowPtr;

static void set_prWindowPtr (struct Window *w)
{
	struct Process *self = (struct Process *) FindTask (NULL);

	if (!saved_prWindowPtr)
		saved_prWindowPtr = self->pr_WindowPtr;
	self->pr_WindowPtr = w;
}

static void restore_prWindowPtr (void)
{
   struct Process *self = (struct Process *) FindTask (NULL);

   if (saved_prWindowPtr)
	self->pr_WindowPtr = saved_prWindowPtr;
}


static APTR setup_classic_buffer (struct vidbuf_description *gfxinfo, const struct RastPort *rp)
{
	APTR buffer;

	gfxinfo->pixbytes= 4;

	int bytes_per_row = gfxinfo->width * gfxinfo->pixbytes;

	// 4 bytes alignment.
	bytes_per_row = bytes_per_row & 3 ? (bytes_per_row & ~3) + 4 : bytes_per_row;

	buffer = AllocVecTagList ( bytes_per_row  * gfxinfo->height , tags_any);
	if (buffer)
	{
		gfxinfo->bufmem      = buffer;
		gfxinfo->rowbytes    = bytes_per_row;
		gfxinfo->flush_line  = flush_line_cgx_v41;
		gfxinfo->flush_block = flush_block_cgx_v41;
	}

	return buffer;
}

void free_picasso_invalid_lines()
{
	if (picasso_invalid_lines)
	{
		FreeVec(picasso_invalid_lines);
		picasso_invalid_lines = NULL;
	}
}

void alloc_picasso_invalid_lines()
{
	picasso_invalid_lines = AllocVecTagList(  picasso_vidinfo.height, tags_any);
	if (picasso_invalid_lines)
	{
		memset (picasso_invalid_lines, 0, picasso_vidinfo.height );
	}
}

static int graphics_subinit_picasso(void)
{
	/* Initialize structure for Picasso96 video modes */

	picasso_vidinfo.rowbytes	= 0;
	picasso_vidinfo.extra_mem = 1;
	picasso_has_invalid_lines	= 0;
	picasso_invalid_start	= picasso_vidinfo.height + 1;
	picasso_invalid_stop	= -1;

	free_picasso_invalid_lines();
	alloc_picasso_invalid_lines();

	return 1;
}

static int graphics_subinit (void)
{

	if (comp_aga_RP.BitMap)
		printf("WTF!! comp_aga_RP.BitMap is not freed\n");

	if (comp_p96_RP.BitMap)
		printf("WTF!! comp_p96_RP.BitMap is not freed\n");

	init_comp(W);

	appw_init (W);
	set_prWindowPtr (W);

	Line = AllocVecTagList ((gfxvidinfo.width + 15) & ~15, tags_public );
	if (!Line)
	{
		write_log ("Unable to allocate raster buffer.\n");
		return 0;
	}

	if (comp_aga_RP.BitMap)
	{
		printf("BitMap was allocated with width %d\n",gfxvidinfo.width);

		BitMap = AllocBitMap (gfxvidinfo.width, 1, 8, BMF_CLEAR | BMF_MINPLANES, comp_aga_RP.BitMap);
		if (!BitMap)
		{
			write_log ("Unable to allocate BitMap.\n");
			return 0;
		}

		TempRPort = AllocVecTagList (sizeof (struct RastPort), tags_public );
		if (!TempRPort)
		{
			write_log ("Unable to allocate RastPort.\n");
			return 0;
		}

		CopyMem (&comp_aga_RP, TempRPort, sizeof (struct RastPort));
		TempRPort->Layer  = NULL;
		TempRPort->BitMap = BitMap;

	}
	else
	{
		write_log ("No aga bitmap.\n");
		return 0;
	}

	if (usepub) set_title ();

	bitdepth = RPDepth (&comp_aga_RP);
	gfxvidinfo.emergmem = 0;
	gfxvidinfo.linemem  = 0;


	CybBuffer = setup_classic_buffer (&gfxvidinfo, &comp_aga_RP);
	if (!CybBuffer)
	{

		/*
		 * Failed to allocate bitmap - we need to fall back on gfx.lib rendering
		 */
		gfxvidinfo.bufmem = NULL;
		output_is_true_color = 0;
		if (bitdepth > 8)
		{
			bitdepth = 8;
			write_log ("AMIGFX: Failed to allocate off-screen buffer - falling back on 8-bit mode\n");
	   	}
	}

	if (is_ham)
	{
		// ham 6 
		use_delta_buffer       = 0; /* needless as the line must be fully recomputed */
		need_dither            = 0;
		gfxvidinfo.pixbytes    = 2;
		gfxvidinfo.flush_line  = flush_line_ham;
		gfxvidinfo.flush_block = flush_block_ham;
	}
	else if (bitdepth <= 8)
	{
		// chunk2planar is slow so we define use_delta_buffer for all modes //
		use_delta_buffer       = 1;
		need_dither            = currprefs.amiga_use_dither || (bitdepth <= 1);
		gfxvidinfo.pixbytes    = need_dither ? 2 : 1;
		gfxvidinfo.flush_line  = need_dither ? flush_line_planar_dither  : flush_line_planar_nodither;
		gfxvidinfo.flush_block = need_dither ? flush_block_planar_dither : flush_block_planar_nodither;
	}

	gfxvidinfo.flush_clear_screen = flush_clear_screen_gfxlib;
	gfxvidinfo.flush_screen = dummy_flush_screen;
	gfxvidinfo.lockscr = dummy_lock;
	gfxvidinfo.unlockscr = dummy_unlock;

	if (!output_is_true_color)
	{
		//
		// We're not using GGX/P96 for output, so allocate a dumb
		// display buffer
		//

		gfxvidinfo.rowbytes = gfxvidinfo.pixbytes * gfxvidinfo.width;
		gfxvidinfo.bufmem   = (uae_u8 *) calloc (gfxvidinfo.rowbytes, gfxvidinfo.height + 1);

		//										  ^^^ //
		//					  This is because DitherLine may read one extra row //
	}

	if (!gfxvidinfo.bufmem)
	{
		write_log ("AMIGFX: Not enough memory for video bufmem.\n");
		return 0;
	}

	if (use_delta_buffer)
	{
		printf("*******  gfxvidinfo.rowbytes: %d\n",gfxvidinfo.rowbytes);

		oldpixbuf = (uae_u8 *) calloc (gfxvidinfo.rowbytes, gfxvidinfo.height);
		if (!oldpixbuf)
		{
			write_log ("AMIGFX: Not enough memory for oldpixbuf.\n");
			return 0;
		}
	}

	gfxvidinfo.maxblocklines = MAXBLOCKLINES_MAX;

	if (!init_colors ())
	{
		write_log ("AMIGFX: Failed to init colors.\n");
		return 0;
	}

	if (screen_is_picasso)
	{
		return graphics_subinit_picasso ();
	}
	else
	{
		reset_drawing ();
		refresh_aga();
	}

	set_default_hotkeys (ami_hotkeys);
	pointer_state = DONT_KNOW;

	return 1;
}

void update_gfxvidinfo_width_height()
{
	gfxvidinfo.width  = currprefs.gfx_width_win;
	gfxvidinfo.height = currprefs.gfx_height_win;

	if (gfxvidinfo.width < 320) gfxvidinfo.width = 320;
	if (!currprefs.gfx_correct_aspect && (gfxvidinfo.width < 64)) gfxvidinfo.width = 200;

	gfxvidinfo.width += 7;
	gfxvidinfo.width &= ~7;
}

int graphics_init (void)
{
	int i, bitdepth;
	use_delta_buffer = 0;
	need_dither = 0;
	output_is_true_color = 0;

	update_gfxvidinfo_width_height();

/*
    currprefs.gfx_correct_aspect;
    currprefs.gfx_afullscreen;
    currprefs.gfx_pfullscreen;
*/

	switch (currprefs.amiga_screen_type)
	{
		case UAESCREENTYPE_ASK:

			if (setup_userscreen ()) break;

			write_log ("Trying on public screen...\n");
			/* fall trough */

		case UAESCREENTYPE_PUBLIC:

			is_halfbrite = 0;
			if (setup_publicscreen ())
			{
				usepub = 1;
				break;
			}
			write_log ("Trying on custom screen...\n");
			/* fall trough */

		case UAESCREENTYPE_CUSTOM:

		default:
			if (!setup_customscreen ())
			return 0;
			break;
	}

	if (graphics_subinit () == 0) 
	{
		write_log ("AMIGFX: subsystem failed.\n");
		return 0;
	}

	if (S) ScreenToFront (S);

	return 1;
}

/****************************************************************************/

void close_window()
{
	if (W)
	{
		restore_prWindowPtr ();
	 	dispose_icon( W, &iconifyIcon );
	 	dispose_icon( W, &padlockicon );
		dispose_icon( W, &fullscreenicon );

		if (is_fullscreen_state == FALSE)
		{
			save_window.x = W -> LeftEdge;
			save_window.y = W -> TopEdge;
			save_window.bw = W -> BorderLeft - W -> BorderRight;
			save_window.bh = W -> BorderTop - W -> BorderBottom;
			save_window.w = W -> Width  ;
			save_window.h = W -> Height  ;
		}

		CloseWindow (W);
		W = NULL;
	}

	free_pointer ();
}

static void graphics_subshutdown (void)
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	appw_exit ();

	if (BitMap)
	{
		WaitBlit ();
		FreeBitMap (BitMap);
		BitMap = NULL;
	}

	if (TempRPort)
	{
		FreeVec (TempRPort);
		TempRPort = NULL;
	}

	if (Line)
	{
		FreeVec (Line);
		Line = NULL;
	}

	if (CybBuffer)
	{
		FreeVec (CybBuffer);
		CybBuffer = NULL;
	}

	if (conv_p96_RP.BitMap)
	{
		FreeBitMap(conv_p96_RP.BitMap);
		conv_p96_RP.BitMap = NULL;
	}

	if (comp_p96_RP.BitMap)
	{
		FreeBitMap(comp_p96_RP.BitMap);
		comp_p96_RP.BitMap = NULL;
	}

	free_picasso_invalid_lines();

	if (comp_aga_RP.BitMap)
	{
		FreeBitMap(comp_aga_RP.BitMap);
		comp_aga_RP.BitMap = NULL;
	}

	if (vpal16)
	{
		FreeVec(vpal16);
		vpal16 = NULL;
	}

	if (vpal32) 
	{
		FreeVec(vpal32);
		vpal32 = NULL;
	}
}

void graphics_leave (void)
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	closepseudodevices ();

	if (CM)
	{
		ReleaseColors();
		CM = NULL;
	}

	graphics_subshutdown();
	close_window();

	if (!usepub && S) 
	{
		if (!CloseScreen (S))
		{
			gui_message ("Please close all opened windows on UAE's screen.\n");
			do
				Delay (50);
			while (!CloseScreen (S));
		}
		S = NULL;
	}

}

/****************************************************************************/

int do_inhibit_frame (int onoff)
{
	if (onoff != -1) 
	{
		inhibit_frame = onoff ? 1 : 0;

		if (inhibit_frame)
			write_log ("display disabled\n");
		else
			write_log ("display enabled\n");
		set_title ();
	}
	return inhibit_frame;
}

/***************************************************************************/

void graphics_notify_state (int state)
{
}

/***************************************************************************/

struct MsgPort *iconifyPort = NULL;
struct DiskObject *_dobj = NULL;
struct AppIcon *appicon = NULL;

bool empty_msg_queue(struct MsgPort *port)
{
	struct Message *msg;
	// empty que.
	while ((msg = (struct Message *) GetMsg( port ) ))
	{
		ReplyMsg( (struct Message *) msg );
		return true;
	}
	return false;
}


bool is_uniconifyed()
{
	if (iconifyPort)
	{
		ULONG signal = 1 << iconifyPort->mp_SigBit;
		 if (SetSignal(0L, signal) & signal)
		{
			return empty_msg_queue(iconifyPort);
		}
	}
	return false;
}

bool enable_Iconify()
{
	int n;

	const char *files[]={"progdir:uae","envarc:sys/def_tool",NULL};

	for (n=0;files[n];n++)
	{
		_dobj = GetDiskObjectNew( files[n] );
		if (_dobj) break;
	}

	if (_dobj)
	{
		_dobj -> do_CurrentX = 0;
		_dobj -> do_CurrentY = 0;

		iconifyPort = (struct MsgPort *) AllocSysObject(ASOT_PORT,NULL);

		if (iconifyPort)
		{
			appicon = AddAppIcon(1, 0, "uae", iconifyPort, 0, _dobj, 
					WBAPPICONA_SupportsOpen, TRUE,
					TAG_END);

			if (appicon) return true;
		}
	}

	return false;
}

void dispose_Iconify()
{
	if (_dobj)
	{
		RemoveAppIcon( appicon );
		FreeDiskObject(_dobj);
		appicon = NULL;
		_dobj = NULL;
	}

	if (iconifyPort)
	{
		FreeSysObject ( ASOT_PORT, iconifyPort ); 
		iconifyPort = NULL;
	}
}


void refresh_aga()
{
	if (use_delta_buffer)
	{
		// hack: this forces refresh //
		uae_u8 *ptr = oldpixbuf;
		int i, len = gfxvidinfo.width;
		len *= gfxvidinfo.pixbytes;
		for (i=0; i < currprefs.gfx_height_win; ++i)
		{
			ptr[00000] ^= 255;
			ptr[len-1] ^= 255;
			ptr += gfxvidinfo.rowbytes;
		}
	}
	BeginRefresh (W);
	flush_block (0, currprefs.gfx_height_win - 1);
	EndRefresh (W, TRUE);
}

void handle_events(void)
{
		struct IntuiMessage *msg;
	int dmx, dmy, mx, my, class, code, qualifier;
	UWORD GadgetID;

   /* this function is called at each frame, so: */
    ++frame_num;       /* increase frame counter */
#if 0
    save_frame();      /* possibly save frame    */
#endif

#ifdef DEBUGGER
    /*
     * This is a hack to simulate ^C as is seems that break_handler
     * is lost when system() is called.
     */

	if (SetSignal (0L, SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D) & (SIGBREAKF_CTRL_C|SIGBREAKF_CTRL_D)) 
	{
		activate_debugger ();
	}
#endif

	if (iconifyPort)		// iconifyed mode..
	{
		if (is_uniconifyed())
		{
			if (save_window.w)
			{
				open_window_as_last();
			}
			else
			{
				open_window();
			}
			dispose_Iconify();
		}

		appw_events();
		return;
	}

	while ( (W) && (msg = (struct IntuiMessage*) GetMsg (W->UserPort)) )
	{
		class     = msg->Class;
		code      = msg->Code;
		dmx       = msg->MouseX;
		dmy       = msg->MouseY;
		mx        = msg->IDCMPWindow->MouseX; // Absolute pointer coordinates
		my        = msg->IDCMPWindow->MouseY; // relative to the window
		qualifier = msg->Qualifier;
	 	GadgetID = (msg -> IAddress) ? ((struct Gadget *) ( msg -> IAddress)) -> GadgetID : 0 ;

		ReplyMsg ((struct Message*)msg);

		switch (class)
		{
			case IDCMP_NEWSIZE:
//				do_inhibit_frame ((W->Flags & WFLG_ZOOMED) ? 1 : 0);
				break;

			case IDCMP_REFRESHWINDOW:

				if (screen_is_picasso == 0) 	refresh_aga();
				break;

			case IDCMP_GADGETUP:
				switch (GadgetID)
				{
					case GID_ICONIFY: 
						if (enable_Iconify())
						{
							empty_msg_queue(W->UserPort);
							close_window();
						}
						break;

					case GID_FULLSCREEN:
						toggle_fullscreen();
						break;

					case GID_PADLOCK:
						toggle_mousegrab();
						break;

					default:
						break;
				}
				break;

			case IDCMP_CLOSEWINDOW:
				uae_quit ();
				break;

		case IDCMP_RAWKEY: {
		int keycode = code & 127;
		int state   = code & 128 ? 0 : 1;
		int ievent;

		if ((qualifier & IEQUALIFIER_REPEAT) == 0) {
			/* We just want key up/down events - not repeats */
			if ((ievent = match_hotkey_sequence (keycode, state)))
			handle_hotkey_event (ievent, state);
			else
			inputdevice_do_keyboard (keycode, state);
		}
		break;
		 }

		case IDCMP_MOUSEMOVE:
		setmousestate (0, 0, dmx, 0);
		setmousestate (0, 1, dmy, 0);

		if (usepub) {
			POINTER_STATE new_state = get_pointer_state (W, mx, my);
			if (new_state != pointer_state) {
			pointer_state = new_state;
			if (pointer_state == INSIDE_WINDOW)
				hide_pointer (W);
			else
				show_pointer (W);
			}
		}
		break;

		case IDCMP_MOUSEBUTTONS:
		if (code == SELECTDOWN) setmousebuttonstate (0, 0, 1);
		if (code == SELECTUP)   setmousebuttonstate (0, 0, 0);
		if (code == MIDDLEDOWN) setmousebuttonstate (0, 2, 1);
		if (code == MIDDLEUP)   setmousebuttonstate (0, 2, 0);
		if (code == MENUDOWN)   setmousebuttonstate (0, 1, 1);
		if (code == MENUUP)     setmousebuttonstate (0, 1, 0);
		break;

		/* Those 2 could be of some use later. */
		case IDCMP_DISKINSERTED:
		/*printf("diskinserted(%d)\n",code);*/
		break;

		case IDCMP_DISKREMOVED:
		/*printf("diskremoved(%d)\n",code);*/
		break;

		case IDCMP_ACTIVEWINDOW:
		/* When window regains focus (presumably after losing focus at some
		 * point) UAE needs to know any keys that have changed state in between.
		 * A simple fix is just to tell UAE that all keys have been released.
		 * This avoids keys appearing to be "stuck" down.
		 */
		inputdevice_acquire ();
		inputdevice_release_all_keys ();
		reset_hotkeys ();

		break;

		case IDCMP_INACTIVEWINDOW:
		inputdevice_unacquire ();
		break;

		case IDCMP_INTUITICKS:
#ifdef __amigaos4__
		grabTicks--;
		if (grabTicks < 0) {
			grabTicks = GRAB_TIMEOUT;
			#ifdef __amigaos4__
			if (mouseGrabbed)
				grab_pointer (W);
			#endif
		}
#endif
		break;

		default:
		write_log ("Unknown event class: %x\n", class);
		break;
        }
    }

    appw_events();
}

/***************************************************************************/

int debuggable (void)
{
    return 1;
}

/***************************************************************************/

int mousehack_allowed (void)
{
    return 0;
}

/***************************************************************************/

void LED (int on)
{
}

/***************************************************************************/

/* sam: need to put all this in a separate module */

#ifdef PICASSO96

/*
 * Add a screenmode to the emulated P96 display database
 */

static void add_p96_mode (int width, int height, int depth, int *count)
{
	unsigned int i;

	if (*count < MAX_PICASSO_MODES)
	{
		DisplayModes[*count].res.width  = width;
		DisplayModes[*count].res.height = height;
		DisplayModes[*count].depth      = depth >> 3;
		DisplayModes[*count].refresh    = 75;
		(*count)++;

		write_log ("AMIGFX: Added P96 mode: %dx%dx%d\n", width, height, depth);
	}
}

BOOL has_p96_mode( uae_u32 width, uae_u32 height, int depth, int max_modes )
{
	struct PicassoResolution *i;

	if (max_modes>MAX_PICASSO_MODES) max_modes = MAX_PICASSO_MODES;
	
//	printf("looking for: %d,%d,%d\n",width,height,depth);

	for (i = DisplayModes; i < DisplayModes + max_modes ; i++ )
	{
//		printf("CHK %d,%d,%d\n",i -> res.width,i -> res.height,i -> depth);

		if ((i -> res.width == width) &&
			(i -> res.height == height) &&
			((i -> depth << 3) == depth)) return TRUE;
	}

	return FALSE;
}


int DX_Blit (int srcx, int srcy, int dstx, int dsty, int width, int height, BLIT_OPCODE opcode)
{
	int result = 0;

	DEBUG_LOG ("DX_Blit (sx:%d sy:%d dx:%d dy:%d w:%d h:%d op:%d)\n", srcx, srcy, dstx, dsty, width, height, opcode);

	if (opcode == BLIT_SRC ) 
	{
		ULONG error = BltBitMapTags(
				BLITA_SrcType, BLITT_BITMAP,
				BLITA_DestType, BLITT_BITMAP,
				BLITA_Source, draw_p96_RP->BitMap,
				BLITA_Dest, draw_p96_RP->BitMap,
				BLITA_SrcX, srcx,
				BLITA_SrcY, srcy,
				BLITA_Width,  width,
				BLITA_Height, height,
				BLITA_DestX, dstx,
				BLITA_DestY, dsty,
				TAG_END);

		if (error == 0)
		{
			DX_Invalidate (dsty, dsty + height - 1);
			result = 1;
		}
	}

	return result;
}

int DX_Fill (int dstx, int dsty, int width, int height, uae_u32 color, RGBFTYPE rgbtype)
{
	DEBUG_LOG ("DX_Fill (x:%d y:%d w:%d h:%d color=%08x)\n", dstx, dsty, width, height, color);

	if (draw_p96_RP -> BitMap)
	{
		RectFillColor(draw_p96_RP, dstx, dsty, dstx + width - 1, dsty + height - 1,color);
		DX_Invalidate (dsty, dsty + height - 1);
		return 1;
	}

	return 0;
}

#define is_hwsurface true

void DX_Invalidate (int first, int last)
{
//    DEBUG_LOG ("Function: DX_Invalidate %d - %d\n", first, last);

	if (!picasso_invalid_lines) return;
	if (first > last) return;

	picasso_has_invalid_lines = 1;
	if (first < picasso_invalid_start) picasso_invalid_start = first;
	if (last > picasso_invalid_stop) picasso_invalid_stop = last;

	while (first <= last)
	{
		picasso_invalid_lines[first] = 1;
		first++;
	}

}

int DX_BitsPerCannon (void)
{
	return 8;
}

void DX_SetPalette (int start, int count)
{
    DEBUG_LOG ("Function: DX_SetPalette\n");

	// exit if not valid !!!
	if (! screen_is_picasso || picasso96_state.RGBFormat != RGBFB_CHUNKY) return;

	if (count > 256) count = 256;

	if (set_palette_fn)	// we need to convert !!
	{
		int n;
		for (n = start ; n<(start+count); n++ )
			set_palette_fn( picasso96_state.CLUT, n );			// fix me !!! wrong size of array maybe!!
	}
	else
	{
		load32_p96_table[ 0 ] = count << 16;

		int i;
		int offset;

		for (i = 0; i < count;  i++)
		{
			offset = (i*3) + 1;
			load32_p96_table[ offset + 0  ] = 0x01010101 * picasso96_state.CLUT[i].Red;
			load32_p96_table[ offset + 1  ] = 0x01010101 * picasso96_state.CLUT[i].Green;
			load32_p96_table[ offset + 2  ] = 0x01010101 * picasso96_state.CLUT[i].Blue;			 
		}

		LoadRGB32( &(S -> ViewPort) , load32_p96_table );
	}
}

void DX_SetPalette_vsync(void)
{
	if (palette_update_end > palette_update_start)
	{
		DX_SetPalette (palette_update_start, palette_update_end - palette_update_start);
		palette_update_end  = 0;
		palette_update_start = 0;
	}
}


void add_native_modes( int depth, int *count )
{
	ULONG ID;
	struct DisplayInfo dispi;
	struct DimensionInfo di;
	int w,h;

	for( ID = NextDisplayInfo( INVALID_ID ) ; ID !=INVALID_ID ;  ID = NextDisplayInfo( ID ) )
	{
		if (
			(GetDisplayInfoData( NULL, &di, sizeof(di) , DTAG_DIMS, ID)) &&
			(GetDisplayInfoData( NULL, &dispi, sizeof(dispi) ,  DTAG_DISP, ID))
		)
		{
			if ( (depth == 15 ? 16: depth) == (di.MaxDepth == 24 ?  32 : di.MaxDepth) )
			{
				w =  di.Nominal.MaxX -di.Nominal.MinX +1;
				h =  di.Nominal.MaxY -di.Nominal.MinY +1;

				if ( has_p96_mode( w,  h, depth, *count ) == FALSE )
				{
					add_p96_mode ( w, h, depth, count);
				}
			}
		}
	}
}

int DX_FillResolutions (uae_u16 *ppixel_format)
{
    int i;
    int count = 0;

    DEBUG_LOG ("Function: DX_FillResolutions\n");

	if (bit_unit == 16)
		picasso_vidinfo.rgbformat = RGBFB_R5G6B5;
	else
		picasso_vidinfo.rgbformat = RGBFB_A8R8G8B8;

	*ppixel_format = 1 << picasso_vidinfo.rgbformat;
	if (bit_unit == 16 || bit_unit == 32)
	{
		*ppixel_format |= RGBFF_CHUNKY;
	}

    /* Check list of standard P96 screenmodes */

	add_native_modes( 32, &count );
	add_native_modes( 15, &count );
	add_native_modes( 8, &count );

	return count;
}

APTR p96_lock = NULL;

uae_u8 *gfx_lock_picasso (void)
{
	APTR address = NULL;

	if (p96_lock) gfx_unlock_picasso ();

	if (draw_p96_RP -> BitMap)
	{
		p96_lock = IGraphics -> LockBitMapTags(draw_p96_RP -> BitMap,
			LBM_BaseAddress, (APTR *) &address,
			LBM_BytesPerRow, &picasso_vidinfo.rowbytes,
			TAG_END	);

		if (!p96_lock)
		{
			DEBUG_LOG ("Function: gfx_lock_picasso, failed to get lock!\n");
			DEBUG_LOG ("Bitmap BytesPerRow: %d, Rows: %d, Depth: %d\n", 
					draw_p96_RP -> BitMap -> BytesPerRow, 
					draw_p96_RP -> BitMap -> Rows, 
					draw_p96_RP -> BitMap -> Depth);
		}
	}

	return address;
}

void gfx_unlock_picasso (void)
{
	DEBUG_LOG ("Function: gfx_unlock_picasso\n");

	if (p96_lock)
	{
		IGraphics -> UnlockBitMap( p96_lock );
		p96_lock = NULL;
	}
}

static void set_window_for_picasso (void)
{
	DEBUG_LOG ("Function: set_window_for_picasso\n");

	if (screen_was_picasso && current_width == picasso_vidinfo.width && current_height == picasso_vidinfo.height)
		return;

	screen_was_picasso = 1;
	graphics_subshutdown();
	current_width  = picasso_vidinfo.width;
	current_height = picasso_vidinfo.height;
	graphics_subinit();
}

void gfx_set_picasso_modeinfo (int w, int h, int depth, int rgbfmt)
{
	DEBUG_LOG ("Function: gfx_set_picasso_modeinfo w: %d h: %d depth: %d rgbfmt: %d\n", w, h, depth, rgbfmt);

	picasso_vidinfo.width = w;
	picasso_vidinfo.height = h;
	picasso_vidinfo.depth = depth;
	picasso_vidinfo.pixbytes = bit_unit >> 3;

	if (screen_is_picasso) set_window_for_picasso();
}

void gfx_set_picasso_state (int on)
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	DEBUG_LOG ("Function: gfx_set_picasso_state: %d\n", on);

	if (on == screen_is_picasso)
	{
		return;
	}

    /* We can get called by drawing_init() when there's
     * no window opened yet... */

	if ( W == NULL)
	{
		printf("no window open... so nothing to do?\n");
		return;
	}

	graphics_subshutdown ();
	screen_was_picasso = screen_is_picasso;
	screen_is_picasso = on;

	if (on)
	{
		// Set height, width for Picasso gfx
		current_width  = picasso_vidinfo.width;
		current_height = picasso_vidinfo.height;
		graphics_subinit ();
	}
	else
	{
		// Set height, width for Amiga gfx
		current_width  = gfxvidinfo.width;
		current_height = gfxvidinfo.height;
		graphics_subinit ();
	}

    if (on) DX_SetPalette (0, 256);
}
#endif

/***************************************************************************/

static int led_state[5];

#define WINDOW_TITLE PACKAGE_NAME " " PACKAGE_VERSION

static void set_title (void)
{
#if 0

	static char title[80];
	static char ScreenTitle[200];

	if (!usepub) return;

	sprintf (title,"%sPower: [%c] Drives: [%c] [%c] [%c] [%c]",
		inhibit_frame? WINDOW_TITLE " (PAUSED) - " : WINDOW_TITLE,
		led_state[0] ? 'X' : ' ',
		led_state[1] ? '0' : ' ',
		led_state[2] ? '1' : ' ',
		led_state[3] ? '2' : ' ',
		led_state[4] ? '3' : ' ');

	sprintf (ScreenTitle,
		"UAE-%d.%d.%d (%s%s%s)  by Bernd Schmidt & contributors, "
                 "Amiga Port by Samuel Devulder.",
		UAEMAJOR, UAEMINOR, UAESUBREV,
		currprefs.cpu_level==0?"68000":
		currprefs.cpu_level==1?"68010":
		currprefs.cpu_level==2?"68020":"68020/68881",
		currprefs.address_space_24?" 24bits":"",
		currprefs.cpu_compatible?" compat":"");

        SetWindowTitles(W, title, ScreenTitle);

#else

	const char *title = inhibit_frame ? WINDOW_TITLE " (Display off)" : WINDOW_TITLE;
	SetWindowTitles (W, title, (char*)-1);

#endif
}

/****************************************************************************/

void main_window_led (int led, int on)                /* is used in amigui.c */
{

#if 0
	if (led >= 0 && led <= 4) led_state[led] = on;
#endif

	set_title ();
}


/****************************************************************************/
/*
 * find the best appropriate color. return -1 if none is available
 */
static LONG ObtainColor (ULONG r,ULONG g,ULONG b)
{
	int i, crgb;
	int colors;

	if (os39 && usepub && CM)
	{
		i = ObtainBestPen (CM, r, g, b,
			OBP_Precision, (use_approx_color ? PRECISION_GUI : PRECISION_EXACT),
			OBP_FailIfBad, TRUE,
			TAG_DONE);
		if (i != -1)
		{
			if (maxpen<256) pen[maxpen++] = i;
			else	i = -1;
		}
		return i;
	}

	colors = is_halfbrite ? 32 : (1 << RPDepth (&comp_aga_RP));

	// private screen => standard allocation //
	if (!usepub)
	{
		if (maxpen >= colors) return -1; // no more colors available //
	
		if (os39)
			SetRGB32 (&S->ViewPort, maxpen, r, g, b);
		else
			SetRGB4 (&S->ViewPort, maxpen, r >> 28, g >> 28, b >> 28);
		return maxpen++;
	}

	// public => find exact match //
	r >>= 28; g >>= 28; b >>= 28;

	if (use_approx_color) return get_nearest_color (r, g, b);

	crgb = (r << 8) | (g << 4) | b;

	for (i = 0; i < colors; i++ )
	{
		int rgb = GetRGB4 (CM, i);

		if (rgb == crgb) return i;
	}
	return -1;
}

/****************************************************************************/
/*
 * free a color entry
 */
static void ReleaseColors(void)
{
    if (os39 && usepub && CM)
	while (maxpen > 0)
		ReleasePen (CM, pen[--maxpen]);
    else
	maxpen = 0;
}

/****************************************************************************/


/****************************************************************************/
/* Here lies an algorithm to convert a 12bits truecolor buffer into a HAM
 * buffer. That algorithm is quite fast and if you study it closely, you'll
 * understand why there is no need for MMX cpu to subtract three numbers in
 * the same time. I can think of a quicker algorithm but it'll need 4096*4096
 * = 1<<24 = 16Mb of memory. That's why I'm quite proud of this one which
 * only need roughly 64Kb (could be reduced down to 40Kb, but it's not
 * worth as I use cidx as a buffer which is 128Kb long)..
 ****************************************************************************/

static int dist4 (LONG rgb1, LONG rgb2) /* computes distance very quickly */
{
    int d = 0, t;
    t = (rgb1&0xF00)-(rgb2&0xF00); t>>=8; if (t<0) d -= t; else d += t;
    t = (rgb1&0x0F0)-(rgb2&0x0F0); t>>=4; if (t<0) d -= t; else d += t;
    t = (rgb1&0x00F)-(rgb2&0x00F); t>>=0; if (t<0) d -= t; else d += t;
#if 0
    t = rgb1^rgb2;
    if(t&15) ++d; t>>=4;
    if(t&15) ++d; t>>=4;
    if(t&15) ++d;
#endif
    return d;
}

#define d_dst (00000+(UBYTE*)cidx) /* let's use cidx as a buffer */
#define d_cmd (16384+(UBYTE*)cidx)
#define h_buf (32768+(UBYTE*)cidx)

static int init_ham (void)
{
    int i,t,RGB;

    /* try direct color first */
    for (RGB = 0; RGB < 4096; ++RGB) {
	int c,d;
	c = d = 50;
	for (i = 0; i < 16; ++i) {
		t = dist4 (i*0x111, RGB);
		if (t<d) {
		d = t;
		c = i;
		}
	}
	i = (RGB & 0x00F) | ((RGB & 0x0F0) << 1) | ((RGB & 0xF00) << 2);
	d_dst[i] = (d << 2) | 3; /* the "|3" is a trick to speedup comparison */
	d_cmd[i] = c;		 /* in the conversion process */
    }
    /* then hold & modify */
    for (i = 0; i < 32768; ++i) {
	int dr, dg, db, d, c;
	dr = (i>>10) & 0x1F; dr -= 0x10; if (dr < 0) dr = -dr;
	dg = (i>>5)  & 0x1F; dg -= 0x10; if (dg < 0) dg = -dg;
	db = (i>>0)  & 0x1F; db -= 0x10; if (db < 0) db = -db;
	c  = 0; d = 50;
	t = dist4 (0,  0*256 + dg*16 + db); if (t < d) {d = t; c = 0;}
	t = dist4 (0, dr*256 +  0*16 + db); if (t < d) {d = t; c = 1;}
	t = dist4 (0, dr*256 + dg*16 +  0); if (t < d) {d = t; c = 2;}
	h_buf[i] = (d<<2) | c;
    }
    return 1;
}

/* great algorithm: convert trucolor into ham using precalc buffers */
#undef USE_BITFIELDS
static void ham_conv (UWORD *src, UBYTE *buf, UWORD len)
{
	union { struct { ULONG _:17,r:5,g:5,b:5;} _; ULONG all;} rgb, RGB;

	rgb.all = 0;

	while(len--)
	{
		UBYTE c,t;
		RGB.all = *src++;
		c = d_cmd[RGB.all];

		// cowabonga! //
		t = h_buf[16912 + RGB.all - rgb.all];

#ifndef USE_BITFIELDS

		if(t<=d_dst[RGB.all])
		{
			static int ht[]={32+10,48+5,16+0}; ULONG m;
			t &= 3; m = 0x1F<<(ht[t]&15);
			m = ~m; rgb.all &= m;
			m = ~m; m &= RGB.all;rgb.all |= m;
			m >>= ht[t]&15;
			c = (ht[t]&~15) | m;
		}
		else
		{
			rgb.all = c;
			rgb.all <<= 5; rgb.all |= c;
			rgb.all <<= 5; rgb.all |= c;
		}
#else
		if(t<=d_dst[RGB.all])
		{
			t&=3;
			if(!t)
			{
				c = 32; c |= (rgb._.r = RGB._.r);
			}
			else 
			{
				--t; if(!t) {c = 48; c |= (rgb._.g = RGB._.g);
			}
			else
			{
				c = 16; c |= (rgb._.b = RGB._.b);}
			}
		}
		else rgb._.r = rgb._.g = rgb._.b = c;
#endif
		*buf++ = c;
	}
}

/****************************************************************************/

int check_prefs_changed_gfx (void)
{
	return 0;
}

/****************************************************************************/



void toggle_mousegrab (void)
{
#ifdef __amigaos4__
    mouseGrabbed = 1 - mouseGrabbed;
    grabTicks    = GRAB_TIMEOUT;
    if (W)
	grab_pointer (W);
#else
    write_log ("Mouse grab not supported\n");
#endif
}

int is_fullscreen (void)
{
    return is_fullscreen_state ? 1: 0;
}

void p96_conv_all()
{
#if 0

	APTR lock_src,lock_dest;
	ULONG src_BytesPerRow,dest_BytesPerRow;
	char *src_buffer_ptr, *dest_buffer_ptr;
	int y;

	if (comp_p96_RP.BitMap == draw_p96_RP -> BitMap)
	{
		printf("royal fuck up... draw bitmap can not be the same as comp bitmap\nwhen converting formats\n");
		return;
	}

	lock_src = IGraphics -> LockBitMapTags(draw_p96_RP -> BitMap,
			LBM_BaseAddress, (APTR *) &src_buffer_ptr,
			LBM_BytesPerRow, &src_BytesPerRow,
			TAG_END	);


	lock_dest = IGraphics -> LockBitMapTags(comp_p96_RP.BitMap,
			LBM_BaseAddress, (APTR *) &dest_buffer_ptr,
			LBM_BytesPerRow, &dest_BytesPerRow,
			TAG_END	);

	if ((lock_src)&&(lock_dest))
	{
		for (y=0;y<picasso_vidinfo.height;y++)
		{
//			if (picasso_invalid_lines[y]) 
			{
				p96_conv_fn( src_buffer_ptr, dest_buffer_ptr, picasso_vidinfo.width );
				picasso_invalid_lines[y] = 0;
			}

			src_buffer_ptr += src_BytesPerRow;
			dest_buffer_ptr += dest_BytesPerRow;
		}
	}

	if (lock_src) IGraphics -> UnlockBitMap( lock_src );
	if (lock_dest) IGraphics -> UnlockBitMap( lock_dest );

#endif
}

int is_vsync (void)
{
	if (W)
	{
		if ((screen_is_picasso) && (comp_p96_RP.BitMap))
		{
			if (p96_conv_fn) p96_conv_all();
			BackFill_Func(NULL, NULL);
		}
		else
		{
			BackFill_Func(NULL, NULL);
		}
	}

	return 0;
}

void toggle_fullscreen (void)
{
	is_fullscreen_state = is_fullscreen_state ? FALSE : TRUE;

	graphics_leave ();
	currprefs.amiga_screen_type = 2;
	notice_screen_contents_lost ();
	XOffset = 0;
	YOffset = 0;
	usepub = 0;
	graphics_setup();
	graphics_init ();
	notice_new_xcolors();
}

void screenshot (int type)
{
    write_log ("Screenshot not implemented yet\n");
}

/****************************************************************************
 *
 * Mouse inputdevice functions
 */

#define MAX_BUTTONS     3
#define MAX_AXES        3
#define FIRST_AXIS      0
#define FIRST_BUTTON    MAX_AXES

static int init_mouse (void)
{
   return 1;
}

static void close_mouse (void)
{
   return;
}

static int acquire_mouse (unsigned int num, int flags)
{
   return 1;
}

static void unacquire_mouse (unsigned int num)
{
   return;
}

static unsigned int get_mouse_num (void)
{
    return 1;
}

static const char *get_mouse_name (unsigned int mouse)
{
    return "Default mouse";
}

static unsigned int get_mouse_widget_num (unsigned int mouse)
{
    return MAX_AXES + MAX_BUTTONS;
}

static int get_mouse_widget_first (unsigned int mouse, int type)
{
    switch (type) {
        case IDEV_WIDGET_BUTTON:
            return FIRST_BUTTON;
        case IDEV_WIDGET_AXIS:
            return FIRST_AXIS;
    }
    return -1;
}

static int get_mouse_widget_type (unsigned int mouse, unsigned int num, char *name, uae_u32 *code)
{
    if (num >= MAX_AXES && num < MAX_AXES + MAX_BUTTONS) {
        if (name)
            sprintf (name, "Button %d", num + 1 + MAX_AXES);
        return IDEV_WIDGET_BUTTON;
    } else if (num < MAX_AXES) {
        if (name)
            sprintf (name, "Axis %d", num + 1);
        return IDEV_WIDGET_AXIS;
    }
    return IDEV_WIDGET_NONE;
}

static void read_mouse (void)
{
    /* We handle mouse input in handle_events() */
}

struct inputdevice_functions inputdevicefunc_mouse = {
    init_mouse,
    close_mouse,
    acquire_mouse,
    unacquire_mouse,
    read_mouse,
    get_mouse_num,
    get_mouse_name,
    get_mouse_widget_num,
    get_mouse_widget_type,
    get_mouse_widget_first
};

/*
 * Default inputdevice config for mouse
 */
void input_get_default_mouse (struct uae_input_device *uid)
{
    /* Supports only one mouse for now */
    uid[0].eventid[ID_AXIS_OFFSET + 0][0]   = INPUTEVENT_MOUSE1_HORIZ;
    uid[0].eventid[ID_AXIS_OFFSET + 1][0]   = INPUTEVENT_MOUSE1_VERT;
    uid[0].eventid[ID_AXIS_OFFSET + 2][0]   = INPUTEVENT_MOUSE1_WHEEL;
    uid[0].eventid[ID_BUTTON_OFFSET + 0][0] = INPUTEVENT_JOY1_FIRE_BUTTON;
    uid[0].eventid[ID_BUTTON_OFFSET + 1][0] = INPUTEVENT_JOY1_2ND_BUTTON;
    uid[0].eventid[ID_BUTTON_OFFSET + 2][0] = INPUTEVENT_JOY1_3RD_BUTTON;
    uid[0].enabled = 1;
}

/****************************************************************************
 *
 * Keyboard inputdevice functions
 */
static unsigned int get_kb_num (void)
{
    return 1;
}

static const char *get_kb_name (unsigned int kb)
{
    return "Default keyboard";
}

static unsigned int get_kb_widget_num (unsigned int kb)
{
    return 128;
}

static int get_kb_widget_first (unsigned int kb, int type)
{
    return 0;
}

static int get_kb_widget_type (unsigned int kb, unsigned int num, char *name, uae_u32 *code)
{
    // fix me
    *code = num;
    return IDEV_WIDGET_KEY;
}

static int keyhack (int scancode, int pressed, int num)
{
    return scancode;
}

static void read_kb (void)
{
}

static int init_kb (void)
{
    return 1;
}

static void close_kb (void)
{
}

static int acquire_kb (unsigned int num, int flags)
{
    return 1;
}

static void unacquire_kb (unsigned int num)
{
}

struct inputdevice_functions inputdevicefunc_keyboard =
{
    init_kb,
    close_kb,
    acquire_kb,
    unacquire_kb,
    read_kb,
    get_kb_num,
    get_kb_name,
    get_kb_widget_num,
    get_kb_widget_type,
    get_kb_widget_first
};

int getcapslockstate (void)
{
    return 0;
}

void setcapslockstate (int state)
{
}

/****************************************************************************
 *
 * Handle gfx specific cfgfile options
 */

static const char *screen_type[] = { "custom", "public", "ask", 0 };

void gfx_default_options (struct uae_prefs *p)
{
	p->amiga_screen_type     = UAESCREENTYPE_PUBLIC;
	p->amiga_publicscreen[0] = '\0';
	p->amiga_use_dither      = 1;
	p->amiga_use_grey        = 0;
}

void gfx_save_options (FILE *f, const struct uae_prefs *p)
{
	cfgfile_write (f, GFX_NAME ".screen_type=%s\n",  screen_type[p->amiga_screen_type]);
	cfgfile_write (f, GFX_NAME ".publicscreen=%s\n", p->amiga_publicscreen);
	cfgfile_write (f, GFX_NAME ".use_dither=%s\n",   p->amiga_use_dither ? "true" : "false");
	cfgfile_write (f, GFX_NAME ".use_grey=%s\n",     p->amiga_use_grey ? "true" : "false");
}

int gfx_parse_option (struct uae_prefs *p, const char *option, const char *value)
{
	return (cfgfile_yesno  (option, value, "use_dither",   &p->amiga_use_dither)
		|| cfgfile_yesno  (option, value, "use_grey",	 &p->amiga_use_grey)
		|| cfgfile_strval (option, value, "screen_type",  &p->amiga_screen_type, screen_type, 0)
		|| cfgfile_string (option, value, "publicscreen", &p->amiga_publicscreen[0], 256));
}

/****************************************************************************/
