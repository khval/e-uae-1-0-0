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

#include <stdbool.h>

#define  __USE_BASETYPE__
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/mpega.h>

#include "custom.h"
#include "newcpu.h"
#include "autoconf.h"
#include "traps.h"

#include "win32_handle_emu.h"
#include "win32_thread_emu.h"

#undef   __USE_BASETYPE__
#include <exec/execbase.h>
#include <workbench/startup.h>

#ifdef USE_SDL
# include <SDL.h>
#endif

struct handle_thread_s main_thread;

#define have_gfxconvert 0

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

#if have_gfxconvert
#include <proto/gfxconvert.h>
#endif

#include <proto/bsdsocket.h>

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
struct Library          *WorkbenchBase = NULL;

struct Library *MpegaBase = NULL;

#if have_gfxconvert
struct Library          *gfxconvertBase = NULL;
struct gfxconvertIFace *Igfxconvert = NULL;
#endif

struct Library          *SocketBase = NULL;

struct AslIFace *IAsl = NULL;
struct GraphicsIFace *IGraphics = NULL;
struct LayersIFace *ILayers = NULL;
struct IntuitionIFace *IIntuition = NULL;
struct CyberGfxIFace *ICyberGfx = NULL;
struct IconIFace *IIcon = NULL;
struct WorkbenchIFace *IWorkbench = NULL;
struct SocketIFace *ISocket = NULL;

struct MpegaIFace *IMpega = NULL;

#include "../gfx-amigaos/window_icons.h"

APTR amiga_thread_safe_mx = NULL;
APTR sigqueue_mx = NULL;

//int thread_triggered_sigbit = 0;

bool remap_initiated = FALSE;

void init_remap_keyboard_logitech_k310( void );
void init_remap_keyboard_AK7000( void );

struct kIcon iconifyIcon = { NULL, NULL };
struct kIcon zoomIcon = { NULL, NULL };
struct kIcon padlockicon = { NULL, NULL };
struct kIcon fullscreenicon = { NULL, NULL };

	#define libOpen(name,ver) \
		name ## Base = OpenLibrary( #name ".library", ver); \
		if (name ## Base) I ## name = (struct name ## IFace *) GetInterface (name ## Base, "main", 1, NULL);

	#define libClose(name) \
		if (I ## name ) DropInterface ((struct Interface *) I ## name ); I ## name = NULL; \
		if (name ## Base) CloseLibrary ( (struct Library *) name ## Base); name ## Base= NULL;

#else

	#define libOpen(name,ver) name ## Base = OpenLibrary ( #name ".library", ver); 
	#define libClose(x) if (x ## Base) CloseLibrary ( x ## Base); x ## Base= NULL;

#endif

int main_task_wakeup_sigbit = -1;
struct Task *main_task = NULL;

#define safe(metod,ptr) if (ptr) { metod(ptr); *ptr = NULL; }

static void free_libs (void)
{
	if (main_task_wakeup_sigbit != -1 )
	{
		FreeSignal(main_task_wakeup_sigbit);
		main_task_wakeup_sigbit = -1;
	}

	if (amiga_thread_safe_mx)
	{
		FreeSysObject( ASOT_MUTEX, amiga_thread_safe_mx);
		amiga_thread_safe_mx = NULL;
	}

	if (sigqueue_mx)
	{
		FreeSysObject( ASOT_MUTEX, sigqueue_mx);
		sigqueue_mx = NULL;
	}


#ifdef __amigaos4__
	if (ITimer) DropInterface ((struct Interface *)ITimer);

	if (timer_device_open)
	{
		CloseDevice((struct IORequest *) &timereq);
		timer_device_open = FALSE;
	}
#endif

	libClose(Socket);
	libClose(Expansion);
	libClose(Asl);
	libClose(Graphics);
	libClose(Layers);
	libClose(Intuition);
	libClose(CyberGfx);
	libClose(Workbench);
	libClose(Icon);

#if have_gfxconvert
	libClose(gfxconvert);
#endif

	libClose(Mpega);
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

	TimerBase = (struct Device *) timereq.Request.io_Device;
	ITimer = (struct TimerIFace *) GetInterface( (struct Library *) TimerBase,"main",1L,NULL) ;

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

#ifndef __AmigaOS4__

	CyberGfxBase = OpenLibrary ("cybergraphics.library", 40);
	if (CyberGfxBase) ICyberGfx = (struct CyberGfxIFace *) GetInterface (CyberGfxBase, "main", 1, NULL);
	if (!ICyberGfx)  return FALSE;

#endif

	SocketBase = OpenLibrary ("bsdsocket.library", 4);
	if (SocketBase) ISocket = (struct SocketIFace *) GetInterface (SocketBase, "main", 1, NULL);
	if (!ISocket)  return FALSE;

	AslBase = OpenLibrary ("asl.library", 53);
	if (AslBase) IAsl = (struct AslIFace *) GetInterface (AslBase, "main", 1, NULL);
	if (!IAsl)  return FALSE;

	WorkbenchBase = OpenLibrary ("workbench.library", 53);
	if (WorkbenchBase) IWorkbench = (struct WorkbenchIFace *) GetInterface (WorkbenchBase, "main", 1, NULL);
	if (!IWorkbench)  return FALSE;

#if have_gfxconvert

	libOpen(gfxconvert,1);
	if (!Igfxconvert)  return FALSE;

#endif

	IconBase = OpenLibrary ("icon.library", 53);
	if (IconBase) IIcon = (struct IconIFace *) GetInterface (IconBase, "main", 1, NULL);
	if (!IIcon)  return FALSE;

	if(!ITimer || !IExpansion) return FALSE;

//	init_remap_keyboard_logitech_k310();
	init_remap_keyboard_AK7000();

#endif

	amiga_thread_safe_mx = AllocSysObjectTags( ASOT_MUTEX, TAG_END );
	if ( ! amiga_thread_safe_mx ) return FALSE;

	sigqueue_mx = AllocSysObjectTags( ASOT_MUTEX, TAG_END );
	if ( ! sigqueue_mx ) return FALSE;

	main_task = FindTask(NULL);
	main_task_wakeup_sigbit = AllocSignal(-1);

	MpegaBase = OpenLibrary("mpega.library", 0);
	if (MpegaBase)
	{
		IMpega = (struct MPEGAIFace *) GetInterface(MpegaBase, "main", 1, NULL);
	}

	return TRUE;
}

char remap_scancode[256];

// US keyboard mapping

void init_remap_keyboard_AK7000( void )
{
	int sc;

	for (sc=0;sc<128;sc++)
	{
		remap_scancode[sc] = sc;
	}
	
	remap_scancode[0x3A]=0x3A;   
	remap_scancode[0x2b]=0x0d; 

	remap_scancode[0x0b]=0x0b;
	remap_scancode[0x0c]=0x0c;

	// map dead keys to closest keys on a win/dos keyboard.

	remap_scancode[112]=0x5F; 	// Home to Help
	remap_scancode[113]=0x5B;	// End to amiga numpad (
	remap_scancode[71]=0x5A;	// insert to amiga numpad )

	// swaped to avoid issues with host system... (Amiga+M etc.)

	remap_scancode[100]=102;	// Left Alt to Left Amiga
	remap_scancode[102]=100;	// Left Window to Left Alt.

	remap_scancode[101]=0x67;	// Right AltGr to Right Amiga
	remap_scancode[103]=0x64;	// Right Window to Right Alt.

	// high bit is used for state...

	for (sc=0;sc<128;sc++)
	{
		remap_scancode[128+sc] = remap_scancode[sc];
	}

	remap_initiated = TRUE;
}

// Norwegian keyboard maping 

void init_remap_keyboard_logitech_k310( void )
{
	int sc;

	for (sc=0;sc<128;sc++)
	{
		remap_scancode[sc] = sc;
	}
	
	remap_scancode[58]=11; //  _- key
	remap_scancode[107]=58; // dead to MENU key

	remap_scancode[11]=12;
	remap_scancode[12]=13;

	// map dead keys to closest keys on a win/dos keyboard.

	remap_scancode[112]=0x5F; 	// Home to Help
	remap_scancode[113]=0x5B;	// End to amiga numpad (
	remap_scancode[71]=0x5A;	// insert to amiga numpad )

	// swaped to avoid issues with host system... (Amiga+M etc.)

	remap_scancode[100]=102;	// Left Alt to Left Amiga
	remap_scancode[102]=100;	// Left Window to Left Alt.

	remap_scancode[101]=0x67;	// Right AltGr to Right Amiga
	remap_scancode[103]=0x64;	// Right Window to Right Alt.

	// high bit is used for state...

	for (sc=0;sc<128;sc++)
	{
		remap_scancode[128+sc] = remap_scancode[sc];
	}

	remap_initiated = TRUE;
}

static int fromWB;
static FILE *logfile;

/*
 * Amiga-specific main entry
 */


int main (int argc, char *argv[])
{
    fromWB = argc == 0;

    init_libs ();

#ifdef USE_SDL
    init_sdl ();
#endif

	if ((argc == 0) && (argv))
	{
		struct WBStartup *wbmsg = (struct WBStartup *) argv;
		struct WBArg	*wargs = wbmsg->sm_ArgList;
		char *newargs[3];
		BPTR	prevlock;

		fromWB = true;
//		set_logfile ("T:E-UAE.log");

		if (wbmsg->sm_NumArgs == 2)
		{
			prevlock	= SetCurrentDir(wargs[1].wa_Lock);
			if (prevlock)
			{
				newargs[0] = "euae";
				newargs[1] = "-f";
				newargs[2] = wargs[1].wa_Name;
				real_main (3, newargs);
				SetCurrentDir(prevlock);	// retsore path.
			}
		}
		else
		{
			struct FileRequester *FileRequest;

			FileRequest = AllocAslRequest (ASL_FileRequest, NULL);
			if (FileRequest)
			{
				if (AslRequestTags (FileRequest,
					ASLFR_TitleText,  "select config",
					ASLFR_InitialDrawer, "progdir:configs",
					ASLFR_InitialPattern, "",
					ASLFR_DoPatterns,   TRUE,
					ASLFR_DoSaveMode,   FALSE,
					ASLFR_RejectIcons,  TRUE,
//					ASLFR_Window,  win,
					TAG_DONE))
				{
					char *path_and_file;

					path_and_file = malloc( strlen(FileRequest->fr_Drawer) + strlen(FileRequest->fr_File) + 2 );

					if (path_and_file)
					{
						strcpy (path_and_file, FileRequest->fr_Drawer);
						if (strlen (path_and_file) && !(path_and_file[strlen (path_and_file) - 1] == ':' || path_and_file[strlen (path_and_file) - 1] == '/')) strcat (path_and_file, "/");
						strcat (path_and_file, FileRequest->fr_File);

						newargs[0] = "euae";
						newargs[1] = "-f";
						newargs[2] = path_and_file;

						currprefs.console_output = 2;

						real_main (3, newargs);

						free(path_and_file);
					}
				}

				FreeAslRequest (FileRequest);
			}
		}
	}
	else
	{
		real_main (argc, argv);
	} 

//    if (fromWB)	set_logfile (0);

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
