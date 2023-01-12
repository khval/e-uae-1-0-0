/*
 * E-UAE - The portable Amiga emulator
 *
 * Copyright 2004-2006 Richard Drummond
 *
 * Start-up and support functions for Amiga target
 */

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "uae.h"
#include "xwin.h"
#include "debug.h"

#include "signal.h"

#include "version.h"

#define  __USE_BASETYPE__
#include <proto/exec.h>
#undef   __USE_BASETYPE__
#include <exec/execbase.h>
#include <proto/wb.h>
#include <proto/timer.h>

#ifdef USE_SDL
# include <SDL.h>
#endif

/* Get compiler/libc to enlarge stack to this size - if possible */
#if defined __PPC__ || defined __ppc__ || defined POWERPC || defined __POWERPC__
# define MIN_STACK_SIZE  (64 * 1024)
#else
# define MIN_STACK_SIZE  (32 * 1024)
#endif

#if defined __libnix__ || defined __ixemul__
/* libnix requires that we link against the swapstack.o module */
unsigned int __stack = MIN_STACK_SIZE;
#else
# if !defined __MORPHOS__ && !defined __AROS__
/* clib2 minimum stack size. Use this on OS3.x and OS4.0. */
unsigned int __stack_size = MIN_STACK_SIZE;
# endif
#endif

struct Device *TimerBase;

//Version tag string for AmigaOS version command
//Not perfect: format of date supposed to be: dd.MM.yyyy, but that format is not available
//at compile time. Could be resolved using a clever define in the makefile...
char* AMIGAOS_VERSION_TAG = "$VER: " UAE_VERSION_STRING " (" __DATE__ ")";

#ifdef __amigaos4__


#include <intuition/imageclass.h>

struct Library *ExpansionBase = NULL;
struct TimerIFace *ITimer = NULL;
struct ExpansionIFace *IExpansion = NULL;
static struct TimeRequest timereq;				// IORequest for timer
BOOL timer_device_open = FALSE;

struct IntuitionBase    *IntuitionBase = NULL;
struct GfxBase          *GraphicsBase = NULL;
struct Library          *LayersBase = NULL;
struct Library          *AslBase = NULL;
struct Library          *CyberGfxBase = NULL;
struct Library          *IconBase = NULL;

struct AslIFace *IAsl = NULL;
struct GraphicsIFace *IGraphics = NULL;
struct LayersIFace *ILayers = NULL;
struct IntuitionIFace *IIntuition = NULL;
struct CyberGfxIFace *ICyberGfx = NULL;
struct IconIFace *IIcon = NULL;

#include "../gfx-amigaos/window_icons.h"

struct kIcon iconifyIcon = { NULL, NULL };
struct kIcon zoomIcon = { NULL, NULL };
struct kIcon padlockicon = { NULL, NULL };
struct kIcon fullscreenicon = { NULL, NULL };

#define closeLib(x) \
	if (I ## x ) DropInterface ((struct Interface *) I ## x ); I ## x = NULL; \
	if (x ## Base) CloseLibrary ( x ## Base); x ## Base= NULL;

#else

#define closeLib(x) if (x ## Base) CloseLibrary ( x ## Base); x ## Base= NULL;

#endif

static void free_libs (void)
{

#ifdef __amigaos4__
	if (ITimer) DropInterface ((struct Interface *)ITimer);

	if (timer_device_open)
	{
		CloseDevice((struct IORequest *) &timereq);
		timer_device_open = FALSE;
	}
#endif

	closeLib(Expansion);
	closeLib(Asl);
	closeLib(Graphics);
	closeLib(Layers);
	closeLib(Intuition);
	closeLib(CyberGfx);
	closeLib(Workbench);
	closeLib(Icon);
}


static BOOL init_libs (void)
{
    atexit (free_libs);

#ifndef __amigaos4__
    TimerBase = (struct Device *) FindName(&SysBase->DeviceList, "timer.device");
#endif

#ifdef __amigaos4__

	if (OpenDevice(TIMERNAME, UNIT_MICROHZ, (struct IORequest *) &timereq, 0))
	{
		return FALSE;
	}

	timer_device_open = TRUE;

	TimerBase = (struct Library *) timereq.Request.io_Device;
	ITimer = (struct TimerIFace *) GetInterface(TimerBase,"main",1L,NULL) ;

	ExpansionBase = OpenLibrary ("expansion.library", 0);
	if (ExpansionBase) IExpansion = (struct ExpansionIFace *) GetInterface(ExpansionBase, "main", 1, 0);

  	IntuitionBase = (void*) OpenLibrary ("intuition.library", 0L);
	if (IntuitionBase) IIntuition = (struct IntuitionIFace *) GetInterface ((struct Library *) IntuitionBase, "main", 1, NULL);
	if (!IIntuition)  return FALSE;

	LayersBase = OpenLibrary ("layers.library", 0L);
	if (LayersBase) ILayers = (struct LayersIFace *) GetInterface (LayersBase, "main", 1, NULL);
	if (!ILayers) return FALSE;

	GraphicsBase = (void*) OpenLibrary ("graphics.library", 0L);
	if (GraphicsBase) IGraphics = (struct GraphicsIFace *) GetInterface ((struct Library *) GraphicsBase, "main", 1, NULL);
	if (!IGraphics) return FALSE;

	CyberGfxBase = OpenLibrary ("cybergraphics.library", 40);
	if (CyberGfxBase) ICyberGfx = (struct CyberGfxIFace *) GetInterface (CyberGfxBase, "main", 1, NULL);
	if (!ICyberGfx)  return FALSE;

	AslBase = OpenLibrary ("asl.library", 53);
	if (AslBase) IAsl = (struct AslIFace *) GetInterface (AslBase, "main", 1, NULL);
	if (!IAsl)  return FALSE;

	WorkbenchBase = OpenLibrary ("workbench.library", 53);
	if (WorkbenchBase) IWorkbench = (struct WorkbenchIFace *) GetInterface (WorkbenchBase, "main", 1, NULL);
	if (!IWorkbench)  return FALSE;

	IconBase = OpenLibrary ("icon.library", 53);
	if (IconBase) IIcon = (struct IconIFace *) GetInterface (IconBase, "main", 1, NULL);
	if (!IIcon)  return FALSE;

	if(!ITimer || !IExpansion) return FALSE;

#endif

	printf("all libs are loaded\n");


	return TRUE;
}

static int fromWB;
static FILE *logfile;

/*
 * Amiga-specific main entry
 */
int main (int argc, char *argv[])
{
    fromWB = argc == 0;

    if (fromWB)
	set_logfile ("T:E-UAE.log");

    init_libs ();

#ifdef USE_SDL
    init_sdl ();
#endif

    real_main (argc, argv);

    if (fromWB)
	set_logfile (0);

    return 0;
}

/*
 * Handle CTRL-C signals
 */
static RETSIGTYPE sigbrkhandler(int foo)
{
#ifdef DEBUGGER
    activate_debugger ();
#endif
}

void setup_brkhandler (void)
{
#ifdef HAVE_SIGACTION
    struct sigaction sa;
    sa.sa_handler = (void*)sigbrkhandler;
    sa.sa_flags = 0;
    sa.sa_flags = SA_RESTART;
    sigemptyset (&sa.sa_mask);
    sigaction (SIGINT, &sa, NULL);
#else
    signal (SIGINT,sigbrkhandler);
#endif
}


/*
 * Handle target-specific cfgfile options
 */
void target_save_options (FILE *f, const struct uae_prefs *p)
{
}

int target_parse_option (struct uae_prefs *p, const char *option, const char *value)
{
    return 0;
}

void target_default_options (struct uae_prefs *p)
{
}
