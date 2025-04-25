/*
 * UAE - The Un*x Amiga Emulator
 *
 * bsdsocket.library emulation - Win32 OS-dependent part
 *
 * Copyright 1997,98 Mathias Ortmann
 * Copyright 1999,2000 Brian King
 *
 * GNU Public License
 *
 */

#include "sysconfig.h"
#include "sysdeps.h"

#include <assert.h>
#include <stddef.h>

#include "options.h"
//include "md-ppc/maccess.h"
#include "include/memory.h"
#include "od-amiga/memory.h"
#include "custom.h"

#include "newcpu.h"
#include "autoconf.h"
#include "traps.h"
#include "threaddep/thread.h"

#ifdef __AMIGAOS4__

	#include <stdbool.h>
	#include <stdint.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <sys/ioctl.h>
	#include <proto/exec.h>
	#include <proto/dos.h>

#define CAN_DO_STACK_MAGIC

#define TRACE(x) Printf x

#endif

#include "bsdsocket.h"


#ifdef __AMIGAOS4__

	#define CRITICAL_SECTION APTR

	APTR threadid;

	extern APTR amiga_thread_safe_mx;
	#define SOCKET_ERROR 1

#undef UINT

	typedef uint32 HWND;
	unsigned int UINT;
	typedef struct Message * MSG;
	typedef unsigned long WPARAM;
	typedef unsigned long LPARAM;

	typedef struct hostent HOSTENT;

	typedef struct 
	{
		int dummy;
	} WSADATA;

	#define __stdcall
	
#include "../od-amiga/win32_handle_emu.h"
#include "../od-amiga/win32_thread_emu.h"
#include "../od-amiga/win32_socket_emu.h"
#include "../od-amiga/win32_event_emu.h"

	#define fd_set _FD_SET

	#define PASCAL
	#define SB struct socketbase *sb
	#define SOCKADDR_IN struct sockaddr_in

	#define WSAEWOULDBLOCK EWOULDBLOCK

	// return signal for etch object type...

ULONG MsgWaitForMultipleObjects( int value, HANDLE *h, bool opt1, bool opt2, uint32 input_opt );

	#define hasEvent(a) ( (a) != NULL)

	struct linger dontlinger = {0,0};

	typedef struct linger LINGER;

	// stuff for some kind of pop window... not sure for what!!!

	#define hAmigaWnd NULL
//	#define hSockWnd NULL
	#define WSABASEERR 0

	// Task stuff.

	#define THREAD_PRIORITY_NORMAL 0
	#define THREAD_PRIORITY_ABOVE_NORMAL 0
	#define THREAD_PRIORITY_TIME_CRITICAL 0

#define CallLib(a,b) CallLib(context,a,b)

BOOL HandleStuff( void );

extern struct Task *main_task;

extern int socket_thread_triggered_sigbit;

uint64 thread_signal_mask = 0;

//int host_CloseSocket(SB, int sd);

uint32 GetLastError( void );

int win32_select_wrapper( long number_of_fds,struct _FD_SET *readsocks, struct _FD_SET *writesocks, struct _FD_SET *exceptsocks, struct timeval *timeout );

	#define THREAD(func,index) new_thread(func,index)
	#define TRIGGER_THREAD { SetEvent( hSockReq ); WaitForSingleObject( hSockReqHandled, INFINITE ); LeaveCriticalSection( &SockThreadCS ); }

	// Stuff I don't know what is...

	#define QS_POSTMESSAGE 0
	#define WM_USER 0
	#define PM_REMOVE 0

	#define EnterCriticalSection(mux) MutexObtain( *(mux) )
	#define LeaveCriticalSection(mux) MutexRelease( *(mux) )


void InitializeCriticalSection( CRITICAL_SECTION *mux)
{
Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	*mux = AllocSysObject(ASOT_MUTEX, TAG_END);
}


static void seterrno(SB, int err)
{
Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	if (sb) sb -> sb_errno = err;
}

	#define Sleep(n) sleep(n)

// don't know how to set the error, no operation for now....
	#define setherrno(sb,errorcode)

int PeekMessage( struct Message **msg, APTR ptr, LONG falgs,  LONG DONT_KNOW, ULONG cmd );
void DispatchMessage( struct Message **msg );

unsigned short MAKEWORD(unsigned char low, unsigned char high);
void DeleteCriticalSection( APTR *lock );
bool WSAStartup( unsigned short flag, APTR ptr );
void WSACleanup( void );
void setWSAAsyncSelect(SB, uae_u32 sd, SOCKET s, long lEvent );
void TranslateMessage( struct Message **msg );

void CloseThread( struct Task *task );
void CloseEvent(struct MsgPort **msg);

bool WSAStartup( unsigned short flag, APTR ptr )
{
	// if error is true..
	return false;	
}

	#define WSAGetLastError() -1

void DeleteCriticalSection( APTR *lock )
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	if (*lock)
	{
	   	FreeSysObject(ASOT_MUTEX, *lock );
		*lock = NULL;
	}
	else
	{
		printf("unexpected NULL pointer\n");
	}

	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
}

int PeekMessage( struct Message **msg, APTR ptr, LONG falgs,  LONG DONT_KNOW, ULONG cmd )
{
Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	*msg = GetMsg( NULL );
	return (*msg) ? 1: 0;
}

void TranslateMessage( struct Message **msg )
{
Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	//  not yet implemented.
	printf("%s:%d - not yet implemented.\n",__FUNCTION__,__LINE__);
}

void DispatchMessage( struct Message **msg )
{
Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	ReplyMsg(*msg);
}

unsigned short MAKEWORD(unsigned char low, unsigned char high)
{
Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	return ((unsigned short) high << 8) | low;
}

void WSACleanup()
{
Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	printf("%s:%d - not yet implemented.\n",__FUNCTION__,__LINE__);
}

// Async Winsock operations

int WSAAsyncSelect(int socket, void *hwnd, unsigned int msg, long event);
long WSAGETSELECTEVENT(long lParam);
long WSAGETSELECTERROR(long lParam);
long WSAGETASYNCERROR(long lParam);
int WSAGETASYNCBUFLEN(long lParam);

int WSAAsyncSelect(int socket, void *hwnd, unsigned int msg, long event)
{
	printf("%s:%d - not yet implemented.\n",__FUNCTION__,__LINE__);
	return 0;
}

long WSAGETSELECTEVENT(long lParam)
{
Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	// Assuming lParam holds event type in lower 16 bits
	return lParam & 0xFFFF;
}

long WSAGETSELECTERROR(long lParam)
{
Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	// Assuming upper 16 bits store the error code
	return (lParam >> 16) & 0xFFFF;
}

long WSAGETASYNCERROR(long lParam)
{
Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	return (lParam >> 16) & 0xFFFF;
}

int WSAGETASYNCBUFLEN(long lParam)
{
Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	return (int)(lParam & 0xFFFF);
}

#else

	#if defined( __GNUC__)
	#define THREAD(func,arg) CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)func,(LPVOID)arg,0,&threadid)
	#else
	#define THREAD(func,arg) _beginthreadex( NULL, 0, func, (void *)arg, 0, (unsigned *)&threadid )
	#endif

	#define hasEvent(a) ( (a) != NULL)

	#define TRIGGER_THREAD { SetEvent( hSockReq ); WaitForSingleObject( hSockReqHandled, INFINITE ); LeaveCriticalSection( &SockThreadCS ); }


#endif


#define BSDSOCKET
#ifdef BSDSOCKET


#include "native2amiga.h"

#define PROTOENT struct protoent
#define SERVENT struct servent 

#include <proto/exec.h>
#include <proto/dos.h>

#undef __USE_INLINE__

#include <proto/bsdsocket.h>

int hWndSelector = 0; /* Set this to zero to get hSockWnd */
CRITICAL_SECTION csSigQueueLock;


#define SETERRNO seterrno(sb,WSAGetLastError()-WSABASEERR)
#define SETHERRNO setherrno(sb,WSAGetLastError()-WSABASEERR)
#define WAITSIGNAL waitsig(context,sb)

#define SETSIGNAL addtosigqueue(sb,0)
#define CANCELSIGNAL cancelsig(context,sb)

#ifndef __amigaos4__	// not needed is in the sdk
#define FIOSETOWN _IOW('f', 124, long)	/* set owner (struct Task *) */
#define FIOGETOWN _IOR('f', 123, long)	/* get owner (struct Task *) */
#endif

#define BEGINBLOCKING if (sb->ftable[sd-1] & SF_BLOCKING) sb->ftable[sd-1] |= SF_BLOCKINGINPROGRESS
#define ENDBLOCKING sb->ftable[sd-1] &= ~SF_BLOCKINGINPROGRESS

static WSADATA wsbData;

int PASCAL WSAEventSelect(SOCKET s,HANDLE h,long x);

#define MAX_SELECT_THREADS 64
HANDLE hThreads[MAX_SELECT_THREADS];
uae_u32 *threadargs[MAX_SELECT_THREADS];
static HANDLE hEvents[MAX_SELECT_THREADS];

#define MAX_GET_THREADS 64
static HANDLE hGetThreads[MAX_GET_THREADS];
uae_u32 *threadGetargs[MAX_GET_THREADS];
static HANDLE hGetEvents[MAX_GET_THREADS];

//void trigger_thread_event( uint32 bit );

uint32 GetLastError()
{
Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	ULONG err_no;
	struct handle_thread_s * thread = (struct handle_thread_s *) GetCurrentThread();

	thread->IS->SocketBaseTags(
		SBTM_SETVAL(SBTC_ERRNOPTR(sizeof(errno))), &err_no,
		TAG_END);

	return err_no;
}

#define clear_trigger_event(  bit ) thread_signal_mask &= ~ ( 1L << bit)

#ifdef __AMIGAOS4__


#define hSockThread hThreads[0]

extern struct Library *SocketBase;


#endif

static HANDLE hSockReq, hSockReqHandled;

static unsigned int __stdcall sock_thread(void *);

CRITICAL_SECTION SockThreadCS;
#define PREPARE_THREAD EnterCriticalSection( &SockThreadCS )


#define SOCKVER_MAJOR 2
#define SOCKVER_MINOR 2

#define SF_RAW_UDP 0x10000000
#define SF_RAW_RAW 0x20000000
#define SF_RAW_RUDP 0x08000000
#define SF_RAW_RICMP 0x04000000

typedef struct ip_option_information {
	u_char Ttl;		/* Time To Live (used for traceroute) */
	u_char Tos; 	/* Type Of Service (usually 0) */
	u_char Flags; 	/* IP header flags (usually 0) */
	u_char OptionsSize; /* Size of options data (usually 0, max 40) */
	u_char FAR *OptionsData;   /* Options data buffer */
} IPINFO, *PIPINFO, FAR *LPIPINFO;


static void bsdsetpriority (HANDLE thread)
{
Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

//	int pri = os_winnt ? THREAD_PRIORITY_NORMAL : priorities[currprefs.win32_active_priority].value;
	int pri = THREAD_PRIORITY_NORMAL;

	SetThreadPriority( thread, pri);
}

#define DWORD uint32

#define WSAVERNOTSUPPORTED ESOCKTNOSUPPORT

static int mySockStartup( void )
{
Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	HANDLE thread = GetCurrentThread();
	int result = 0;
	SOCKET dummy;
	DWORD lasterror;


	if (!thread)
	{
		printf("thread not found\n");
		return 0;
	}

	if (!((struct handle_thread_s *) thread) -> IS)
	{
		printf("no interface\n");
		return 0;
	}

	if (WSAStartup(MAKEWORD( SOCKVER_MAJOR, SOCKVER_MINOR ), &wsbData))
	{
//		lasterror = WSAGetLastError();

		win32_SocketBaseTags(SBTM_GETVAL(SBTC_ERRNO), &lasterror );

		if( lasterror == WSAVERNOTSUPPORTED )
		{
//			char szMessage[ MAX_DPATH ];
//			WIN32GUI_LoadUIString( IDS_WSOCK2NEEDED, szMessage, MAX_DPATH );
//				gui_message( szMessage );
		}
		else
			write_log ( "BSDSOCK: ERROR - Unable to initialize Windows socket layer! Error code: %d\n", lasterror );

		return 0;
	}

Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	if( ( dummy = win32_socket( AF_INET,SOCK_STREAM,IPPROTO_TCP ) ) != INVALID_SOCKET )
	{

Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

		win32_closesocket( dummy );
		result = 1;
	}
	else
	{

Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

		write_log ( "BSDSOCK: ERROR - WSPStartup/NSPStartup failed! Error code: %d\n",WSAGetLastError() );
		result = 0;
	}

	return result;
}

static int socket_layer_initialized = 0;

#define isNULL(obj) (obj  == NULL)
#define isNotNULL(obj) (obj != NULL)

int init_socket_layer(void)
{
	int result = 0;

Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

#ifndef CAN_DO_STACK_MAGIC
	printf("this crap!! \n");
	currprefs.socket_emu = 0;
#endif

	if( currprefs.socket_emu )
	{

Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

		if( ( result = mySockStartup() ) )
		{

Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

			InitializeCriticalSection(&csSigQueueLock);

			if( isNULL( hSockThread ) )
			{

Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

#if 0
				WNDCLASS wc;	// Set up an invisible window and dummy wndproc
#endif
				InitializeCriticalSection( &SockThreadCS );

Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

				hSockReq = CreateEvent( NULL, FALSE, FALSE, NULL );
				hSockReqHandled = CreateEvent( NULL, FALSE, FALSE, NULL );
#if 0
				wc.style = CS_BYTEALIGNCLIENT | CS_BYTEALIGNWINDOW;
				wc.lpfnWndProc = SocketWindowProc;
				wc.cbClsExtra = 0;
				wc.cbWndExtra = 0;
				wc.hInstance = 0;
//				wc.hIcon = LoadIcon (GetModuleHandle (NULL), MAKEINTRESOURCE (IDI_APPICON));
				wc.hCursor = LoadCursor (NULL, IDC_ARROW);
				wc.hbrBackground = GetStockObject (BLACK_BRUSH);
				wc.lpszMenuName = 0;
				wc.lpszClassName = "SocketFun";


				if( RegisterClass (&wc) )
				{

Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

					hSockWnd = CreateWindowEx ( 0,
						"SocketFun", "WinUAE Socket Window",
						WS_POPUP,
						0, 0,
						1, 1,
						NULL, NULL, 0, NULL);
#endif

Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

					hSockThread = THREAD( sock_thread, 0);
#if 0
				}
#endif
			}
		}
	}

	socket_layer_initialized = result;

	return result;
}

void deinit_socket_layer(void)
{
	int i;

Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

printf("currprefs.socket_emu: %d\n", currprefs.socket_emu);

	if( currprefs.socket_emu )
	{
		WSACleanup();
		if( socket_layer_initialized )
		{
			DeleteCriticalSection( &csSigQueueLock );

			if( isNotNULL( hSockThread ) )
			{
				DeleteCriticalSection( &SockThreadCS );

				CloseHandle( hSockReq );
				hSockReq = NULL;

				CloseHandle( hSockReqHandled );
				WaitForSingleObject( hSockThread, INFINITE );

				CloseHandle( hSockThread );

			}
			for (i = 0; i < MAX_SELECT_THREADS; i++)
			{
				if (isNotNULL( hThreads[i] ) )
				{
					CloseHandle( hThreads[i] );
				}
			}
		}
	}
}

#ifdef BSDSOCKET

void locksigqueue(void)
{
Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	EnterCriticalSection(&csSigQueueLock);

Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
}

void unlocksigqueue(void)
{
Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	LeaveCriticalSection(&csSigQueueLock);

Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

}

// Asynchronous completion notification

// We use window messages posted to hAmigaWnd in the range from 0xb000 to 0xb000+MAXPENDINGASYNC*2
// Socket events cause even-numbered messages, task events odd-numbered messages
// Message IDs are allocated on a round-robin basis and deallocated by the main thread.

// WinSock tends to choke on WSAAsyncCancelMessage(s,w,m,0,0) called too often with an event pending

// @@@ Enabling all socket event messages for every socket by default and basing things on that would
// be cleaner (and allow us to write a win32_select() emulation that doesn't need to be kludge-aborted).
// However, the latency of the message queue is too high for that at the moment (setting up a dummy
// window from a separate thread would fix that).

// Blocking sockets with asynchronous event notification are currently not safe to use.


struct socketbase *asyncsb[MAXPENDINGASYNC];
SOCKET asyncsock[MAXPENDINGASYNC];
uae_u32 asyncsd[MAXPENDINGASYNC];
int asyncindex;


int host_sbinit (TrapContext *context, SB)
{
Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	HANDLE thread = GetCurrentThread();

Printf("%s:%s:%ld -- sb: %p, thread: %p\n",__FILE__,__FUNCTION__,__LINE__, sb, thread);

	sb->sockAbort = win32_socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);

	if (sb->sockAbort == (unsigned int) INVALID_SOCKET) return 0;

	sb->hEvent = CreateEvent(NULL,FALSE,FALSE,NULL);

	if ( ! hasEvent( sb->hEvent = CreateEvent(NULL,FALSE,FALSE,NULL)) ) return 0;

	sb->mtable = calloc(sb->dtablesize,sizeof(*sb->mtable));

Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	return 1;
}

void host_closesocketquick(int s)
{
Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	if( s )
	{
		HANDLE thread = GetCurrentThread();

#ifdef __amigaos4__
		win32_setsockopt((SOCKET)s,SOL_SOCKET,SO_LINGER,(char *)&dontlinger,sizeof(dontlinger));
#else
		BOOL true = 1;
		win32_setsockopt((SOCKET)s,SOL_SOCKET,SO_DONTLINGER,(char *)&true,sizeof(true));
#endif
		win32_shutdown(s,1);
		win32_closesocket((SOCKET)s);
	}
}

void host_sbcleanup(SB)
{
Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	int i;
	HANDLE thread = GetCurrentThread();

	for (i = 0; i < MAXPENDINGASYNC; i++) if (asyncsb[i] == sb) asyncsb[i] = NULL;

	if (hasEvent(sb->hEvent)) CloseHandle( sb->hEvent );

	for (i = sb->dtablesize; i--; )
	{
		if (sb->dtable[i] != (int)INVALID_SOCKET) host_closesocketquick(sb->dtable[i]);
		if (sb->mtable[i]) asyncsb[(sb->mtable[i]-0xb000)/2] = NULL;
	}

	win32_shutdown(sb->sockAbort,1);
	win32_closesocket(sb->sockAbort);

	free(sb->mtable);
}

void host_sbreset(void)
{
Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	memset(asyncsb,0,sizeof asyncsb);
	memset(asyncsock,0,sizeof asyncsock);
	memset(asyncsd,0,sizeof asyncsd);
	memset(threadargs,0,sizeof threadargs);
}

void sockmsg(unsigned int msg, unsigned long wParam, unsigned long lParam)
{
Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	SB;
	unsigned int index;
	int sdi;

	index = (msg-0xb000)/2;
	sb = asyncsb[index];

	if (!(msg & 1))
	{
		// is this one really for us?
		if ((SOCKET)wParam != asyncsock[index])
		{
			// cancel socket event
//			WSAAsyncSelect((SOCKET)wParam,hWndSelector ? hAmigaWnd : hSockWnd,0,0);
			return;
		}

		sdi = asyncsd[index]-1;

		// asynchronous socket event?

		if (sb && !(sb->ftable[sdi] & SF_BLOCKINGINPROGRESS) && sb->mtable[sdi])
		{
			long wsbevents = WSAGETSELECTEVENT(lParam);
			int fmask = 0;

			// regular socket event?
			if (wsbevents & FD_READ) fmask = REP_READ;
			else if (wsbevents & FD_WRITE) fmask = REP_WRITE;
			else if (wsbevents & FD_OOB) fmask = REP_OOB;
			else if (wsbevents & FD_ACCEPT) fmask = REP_ACCEPT;
			else if (wsbevents & FD_CONNECT) fmask = REP_CONNECT;
			else if (wsbevents & FD_CLOSE) fmask = REP_CLOSE;

			// error?
			if (WSAGETSELECTERROR(lParam)) fmask |= REP_ERROR;

			// notify
			if (sb->ftable[sdi] & fmask) sb->ftable[sdi] |= fmask<<8;

			addtosigqueue(sb,1);
			return;
		}
	}

	locksigqueue();

	if (sb != NULL)
	{
		asyncsb[index] = NULL;

		if (WSAGETASYNCERROR(lParam))
		{
			seterrno(sb,WSAGETASYNCERROR(lParam)-WSABASEERR);
			if (sb->sb_errno >= 1001 && sb->sb_errno <= 1005) setherrno(sb,sb->sb_errno-1000);
			else if (sb->sb_errno == 55)	// ENOBUFS
				write_log ("BSDSOCK: ERROR - Buffer overflow - %d bytes requested\n",WSAGETASYNCBUFLEN(lParam));
		}
		else seterrno(sb,0);


		SETSIGNAL;
	}

	unlocksigqueue();
}

static unsigned int allocasyncmsg(SB,uae_u32 sd,SOCKET s)
{
Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	int i;
	locksigqueue();

	for (i = asyncindex+1; i != asyncindex; i++)
	{
		if (i == MAXPENDINGASYNC) i = 0;

		if (!asyncsb[i])
		{
			asyncsb[i] = sb;
			if (++asyncindex == MAXPENDINGASYNC) asyncindex = 0;
			unlocksigqueue();

			if (s == INVALID_SOCKET)
			{
				return i*2+0xb001;
			}
			else
			{
				asyncsd[i] = sd;
				asyncsock[i] = s;
				return i*2+0xb000;
			}
		}
	}

	unlocksigqueue();

	seterrno(sb,12); // ENOMEM
	write_log ("BSDSOCK: ERROR - Async operation completion table overflow\n");

	return 0;
}

// static void cancelasyncmsg(unsigned int wMsg)	// org
static void cancelasyncmsg( struct TrapContext *context, unsigned int wMsg) // added context... don't remember why...
{
Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	SB;
	wMsg = (wMsg-0xb000)/2;

	sb = asyncsb[wMsg];

	if (sb != NULL)
	{
		asyncsb[wMsg] = NULL;
		CANCELSIGNAL;	// needs context, so cancelasyncmsg must have context.
	}
}

void sockabort(SB)
{
Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	locksigqueue();

	unlocksigqueue();
}

void setWSAAsyncSelect(SB, uae_u32 sd, SOCKET s, long lEvent )
{
Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	if (sb->mtable[sd-1])
	{
		long wsbevents = 0;
		long eventflags;
		int i;

		locksigqueue();
		eventflags = sb->ftable[sd-1]  & REP_ALL;

		if (eventflags & REP_ACCEPT) wsbevents |= FD_ACCEPT;
		if (eventflags & REP_CONNECT) wsbevents |= FD_CONNECT;
		if (eventflags & REP_OOB) wsbevents |= FD_OOB;
		if (eventflags & REP_READ) wsbevents |= FD_READ;
		if (eventflags & REP_WRITE) wsbevents |= FD_WRITE;
		if (eventflags & REP_CLOSE) wsbevents |= FD_CLOSE;
		wsbevents |= lEvent;


		i = (sb->mtable[sd-1]-0xb000)/2;
		asyncsb[i] = sb;
		asyncsd[i] = sd;
		asyncsock[i] = s;

//		WSAAsyncSelect(s,hWndSelector ? hAmigaWnd : hSockWnd,sb->mtable[sd-1],wsbevents);

		unlocksigqueue();
	}
}


// address cleaning
static void prephostaddr(SOCKADDR_IN *addr)
{
Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	addr->sin_family = AF_INET;
}

static void prepamigaaddr(struct sockaddr *realpt, int len)
{
Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	// little endian address family value to the byte sin_family member
	((char *)realpt)[1] = *((char *)realpt);

	// set size of address
	*((char *)realpt) = len;
}


int host_dup2socket(SB, int fd1, int fd2)
{
Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	HANDLE thread = GetCurrentThread();
	SOCKET s1,s2;

	TRACE(("dup2socket(%d,%d) -> ",fd1,fd2));
	fd1++;

	s1 = getsock(sb, fd1);
	if (s1 != INVALID_SOCKET)
	{
		if (fd2 != -1)
		{
			if ((unsigned int) (fd2) >= (unsigned int) sb->dtablesize)
			{
				TRACE (("Bad file descriptor (%d)\n", fd2));
				seterrno (sb, 9);	/* EBADF */
			}
			fd2++;
			s2 = getsock(sb,fd2);
			if (s2 != INVALID_SOCKET)
			{
				win32_shutdown(s2,1);
				win32_closesocket(s2);
			}
			setsd(sb,fd2,s1);
			TRACE(("0\n"));
			return 0;
		}
		else
		{
			fd2 = getsd(sb, 1);
			setsd(sb,fd2,s1);
			TRACE(("%d\n",fd2));
			return (fd2 - 1);
		}
	}
	TRACE(("-1\n"));
	return -1;
}

int host_socket(SB, int af, int type, int protocol)
{
Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	HANDLE thread = GetCurrentThread();
	int sd;
	SOCKET s;
	unsigned long nonblocking = 1;

	TRACE(("win32_socket(%s,%s,%d) -> ",af == AF_INET ? "AF_INET" : "AF_other",type == SOCK_STREAM ? "SOCK_STREAM" : type == SOCK_DGRAM ? "SOCK_DGRAM " : "SOCK_RAW",protocol));

	if ((s = win32_socket(af,type,protocol)) == INVALID_SOCKET)
	{
		SETERRNO;
		TRACE(("failed (%d)\n",sb->sb_errno));
		return -1;
	}
	else
		sd = getsd(sb,(int)s);

	sb->ftable[sd-1] = SF_BLOCKING;
		win32_IoctlSocket(s,FIONBIO,&nonblocking);

	TRACE(("%d\n",sd));

	if (type == SOCK_RAW)
	{
		if (protocol==IPPROTO_UDP)
		{
			sb->ftable[sd-1] |= SF_RAW_UDP;
		}
		if (protocol==IPPROTO_ICMP)
		{
			struct sockaddr_in sin;

			sin.sin_family = AF_INET;
			sin.sin_addr.s_addr = INADDR_ANY;
			win32_bind(s,(struct sockaddr *)&sin,sizeof(sin)) ;
		}
		if (protocol==IPPROTO_RAW)
		{
			sb->ftable[sd-1] |= SF_RAW_RAW;
		}
	}

	return sd-1;
}

uae_u32 host_bind(SB, uae_u32 sd, uae_u32 name, uae_u32 namelen)
{
Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	HANDLE thread = GetCurrentThread();
	char buf[MAXADDRLEN];
	uae_u32 success = 0;
	SOCKET s;

	sd++;
	TRACE(("win32_bind(%d,0x%lx,%d) -> ",sd,name,namelen));
	s = getsock(sb, sd);

	if (s != INVALID_SOCKET)
	{
		if (namelen <= sizeof buf)
		{
			memcpy(buf,get_real_address (name),namelen);

			// some Amiga programs set this field to bogus values
			prephostaddr((SOCKADDR_IN *)buf);

			if ((success = win32_bind(s,(struct sockaddr *)buf,namelen)) != 0)
			{
				SETERRNO;
				TRACE(("failed (%d)\n",sb->sb_errno));
			}
			else
			{
				TRACE(("OK\n"));
			}
		}
		else
		{
			write_log ("BSDSOCK: ERROR - Excessive namelen (%d) in win32_bind()!\n",namelen);
		}
	}

	return success;
}

uae_u32 host_listen(SB, uae_u32 sd, uae_u32 backlog)
{
Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	HANDLE thread = GetCurrentThread();
	SOCKET s;
	uae_u32 success = -1;

	sd++;
	TRACE(("win32_listen(%d,%d) -> ",sd,backlog));
	s = getsock(sb, sd);

	if (s != INVALID_SOCKET)
	{
		if ((success = win32_listen(s,backlog)) != 0)
		{
			SETERRNO;
			TRACE(("failed (%d)\n",sb->sb_errno));
		}
		else
			TRACE(("OK\n"));
	}

	return success;
}

void host_accept(TrapContext *context,SB, uae_u32 sd, uae_u32 name, uae_u32 namelen)
{
Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	HANDLE thread = GetCurrentThread();
	struct sockaddr *rp_name,*rp_nameuae;
	struct sockaddr sockaddr;
	unsigned int hlen, hlenuae=0;
	SOCKET s, s2;
	int success = 0;
	unsigned int wMsg;

	sd++;
	if (name != 0 )
		{
		rp_nameuae = rp_name = (struct sockaddr *)get_real_address (name);
		hlenuae = hlen = get_long (namelen);
		if (hlenuae < sizeof(sockaddr))
			{ // Fix for CNET BBS Windows must have 16 Bytes (sizeof(sockaddr)) otherwise Error WSAEFAULT
			rp_name = &sockaddr;
			hlen = sizeof(sockaddr);
			}
		}
	else
		{
		rp_name = &sockaddr;
		hlen = sizeof(sockaddr);
		}
	TRACE(("win32_accept(%d,%d,%d) -> ",sd,name,hlenuae));

	s = (SOCKET)getsock(sb,(int)sd);

	if (s != INVALID_SOCKET)
	{
		BEGINBLOCKING;

		s2 = win32_accept(s,rp_name,&hlen);

		if (s2 == INVALID_SOCKET)
		{
			SETERRNO;

			if (sb->ftable[sd-1] & SF_BLOCKING && sb->sb_errno == WSAEWOULDBLOCK-WSABASEERR)
			{
				if (sb->mtable[sd-1] || (wMsg = allocasyncmsg(sb,sd,s)) != 0)
				{
					if (sb->mtable[sd-1] == 0)
					{
//						WSAAsyncSelect(s,hWndSelector ? hAmigaWnd : hSockWnd,wMsg,FD_ACCEPT);
					}
					else
					{
						setWSAAsyncSelect(sb,sd,s,FD_ACCEPT);
					}

					WAITSIGNAL;

					if (sb->mtable[sd-1] == 0)
					{
						cancelasyncmsg(context, wMsg);
					}
					else
					{
						setWSAAsyncSelect(sb,sd,s,0);
					}

					if (sb->eintr)
					{
						TRACE(("[interrupted]\n"));
						ENDBLOCKING;
						return;
					}

					s2 = win32_accept(s,rp_name,&hlen);

					if (s2 == INVALID_SOCKET)
					{
						SETERRNO;

						if (sb->sb_errno == WSAEWOULDBLOCK-WSABASEERR) write_log ("BSDSOCK: ERRRO - win32_accept() would block despite FD_ACCEPT message\n");
					}
				}
			}
		}

		if (s2 == INVALID_SOCKET)
		{
			sb->resultval = -1;
			TRACE(("failed (%d)\n",sb->sb_errno));
		}
		else
		{
			sb->resultval = getsd(sb, s2);
			sb->ftable[sb->resultval-1] = sb->ftable[sd-1];	// new socket inherits the old socket's properties
			sb->resultval--;

				if (rp_name != 0)
			{ // 1.11.2002 XXX
				if (hlen <= hlenuae)
				{ // Fix for CNET BBS Part 2
					prepamigaaddr(rp_name,hlen);
					if (namelen != 0)
					{
						put_long (namelen,hlen);
					}
				}
				else
				{ // Copy only the number of bytes requested
					if (hlenuae != 0)
					{
						prepamigaaddr(rp_name,hlenuae);
						memcpy(rp_nameuae,rp_name,hlenuae);
						put_long (namelen,hlenuae);
						}
					}
				}
			TRACE(("%d/%d\n",sb->resultval,hlen));
		}

		ENDBLOCKING;
	}
}

typedef enum
{
	connect_req,
	recvfrom_req,
	sendto_req,
	abort_req,
	last_req
} threadsock_e;

struct threadsock_packet
{
	threadsock_e packet_type;
	union
	{
	struct sendto_params
	{
		char *buf;
		char *realpt;
		uae_u32 sd;
		uae_u32 msg;
		uae_u32 len;
		uae_u32 flags;
		uae_u32 to;
		uae_u32 tolen;
	} sendto_s;
	struct recvfrom_params
	{
		char *realpt;
		uae_u32 addr;
		uae_u32 len;
		uae_u32 flags;
		struct sockaddr *rp_addr;
		unsigned int *hlen;
	} recvfrom_s;
	struct connect_params
	{
		char *buf;
		uae_u32 namelen;
	} connect_s;
	struct abort_params
	{
		SOCKET *newsock;
	} abort_s;
	} params;
	SOCKET s;
	SB;
} sockreq;

BOOL HandleStuff( void )
{
Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	HANDLE thread = GetCurrentThread();
	BOOL quit = FALSE;
	SB = NULL;
	BOOL handled = TRUE;
	if( hSockReq )
	{
		// 100ms sleepiness might need some tuning...
		//if(WaitForSingleObject( hSockReq, 100 ) == WAIT_OBJECT_0 )
		{
			switch( sockreq.packet_type )
			{
				case connect_req:
					sockreq.sb->resultval = win32_connect(sockreq.s,(struct sockaddr *)(sockreq.params.connect_s.buf),sockreq.params.connect_s.namelen);
					break;

				case sendto_req:
					if( sockreq.params.sendto_s.to )
					{
						sockreq.sb->resultval = win32_sendto(sockreq.s,sockreq.params.sendto_s.realpt,sockreq.params.sendto_s.len,sockreq.params.sendto_s.flags,(struct sockaddr *)(sockreq.params.sendto_s.buf),sockreq.params.sendto_s.tolen);
					}
					else
					{
						sockreq.sb->resultval = win32_send(sockreq.s,sockreq.params.sendto_s.realpt,sockreq.params.sendto_s.len,sockreq.params.sendto_s.flags);
					}
					break;

				case recvfrom_req:
					if( sockreq.params.recvfrom_s.addr )
					{
						sockreq.sb->resultval = win32_recvfrom( sockreq.s, sockreq.params.recvfrom_s.realpt, sockreq.params.recvfrom_s.len,
							  sockreq.params.recvfrom_s.flags, sockreq.params.recvfrom_s.rp_addr,
							  sockreq.params.recvfrom_s.hlen );

					}
					else
					{
						sockreq.sb->resultval = win32_recv( sockreq.s, sockreq.params.recvfrom_s.realpt, sockreq.params.recvfrom_s.len,
							  sockreq.params.recvfrom_s.flags );
					}
					break;

				case abort_req:
					*(sockreq.params.abort_s.newsock) = win32_socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);

					if (*(sockreq.params.abort_s.newsock) !=  (SOCKET) sb->sockAbort)
					{
						win32_shutdown( sb->sockAbort, 1 );
						win32_closesocket( sb->sockAbort );
					}

					handled = FALSE; /* Don't bother the SETERRNO section after the switch() */
					break;

				case last_req:

				default:
					write_log ( "BSDSOCK: Invalid sock-thread request!\n" );
					handled = FALSE;
					break;

			}
			if( handled )
			{
				if( sockreq.sb->resultval == SOCKET_ERROR )
				{
					sb = sockreq.sb;

					SETERRNO;
				}
			}

			SetEvent( hSockReqHandled );
		}
	}
	else
	{
		quit = TRUE;
	}
	return quit;
}

#ifdef __amigaos4__

static long DefWindowProc( HWND hwnd, uint32 message, WPARAM wParam, LPARAM lParam )
{
	printf("%s:%d - not yet implemented.\n",__FUNCTION__,__LINE__);
	return 0;
}

static long SocketWindowProc( HWND hwnd, uint32 message, WPARAM wParam, LPARAM lParam )
#else
static long FAR PASCAL SocketWindowProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
#endif
{
Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	if( message >= 0xB000 && message < 0xB000+MAXPENDINGASYNC*2 )
	{
#if DEBUG_SOCKETS
		write_log ( "sockmsg(0x%x, 0x%x, 0x%x)\n", message, wParam, lParam );
#endif
		sockmsg( message, wParam, lParam );
		return 0;
	}

	return DefWindowProc( hwnd, message, wParam, lParam );
}


static unsigned int __stdcall sock_thread(void *blah)
{
Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	unsigned int result = 0;
	HANDLE WaitHandle[2];
	MSG msg;

Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);


		// Make sure we're outrunning the wolves
		int pri = THREAD_PRIORITY_ABOVE_NORMAL;
#if 0
		if (!os_winnt) {
		pri = priorities[currprefs.win32_active_priority].value;
		if (pri == THREAD_PRIORITY_HIGHEST)
			pri = THREAD_PRIORITY_TIME_CRITICAL;
		else
			pri++;
		}
#endif

Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

		SetThreadPriority( GetCurrentThread(), pri );

Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

		while( TRUE )
		{

Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

			if( hSockReq )
			{
				DWORD wait;
				WaitHandle[0] = hSockReq;	// only socket req is in the array...

Printf("%s:%s:%ld -- before MsgWaitForMultipleObjects(...)\n",__FILE__,__FUNCTION__,__LINE__);

				wait = MsgWaitForMultipleObjects (1, WaitHandle, FALSE,INFINITE, QS_POSTMESSAGE);

				if (wait == WAIT_OBJECT_0) // default network stuff..
				{
					if( HandleStuff() ) // See if its time to quit...
						break;
				}

				if (wait == WAIT_OBJECT_1)	// GUI Stuff...
				{
					Sleep(10);
					while( PeekMessage( &msg, NULL, WM_USER, 0xB000+MAXPENDINGASYNC*2, PM_REMOVE ) > 0 )
					{
						TranslateMessage( &msg );
						DispatchMessage( &msg );
					}
				}
			}
		}


#ifndef __GNUC__
	_endthreadex( result );
#endif

	return result;
}


void host_connect(TrapContext *context, SB, uae_u32 sd, uae_u32 name, uae_u32 namelen)
{
Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	SOCKET s;
	int success = 0;
	unsigned int wMsg;
	char buf[MAXADDRLEN];


	sd++;
	TRACE(("win32_connect(%d,0x%lx,%d) -> ",sd,name,namelen));

	s = (SOCKET)getsock(sb,(int)sd);

	if (s != INVALID_SOCKET)
	{

#ifndef __amigaos4__

		if (namelen <= MAXADDRLEN)
		{
			if (sb->mtable[sd-1] || (wMsg = allocasyncmsg(sb,sd,s)) != 0)
			{
				if (sb->mtable[sd-1] == 0)
				{
					WSAAsyncSelect(s,hWndSelector ? hAmigaWnd : hSockWnd,wMsg,FD_CONNECT);
				}
				else
				{
					setWSAAsyncSelect(sb,sd,s,FD_CONNECT);
				}

				BEGINBLOCKING;
				PREPARE_THREAD;

				memcpy(buf,get_real_address (name),namelen);
				prephostaddr((SOCKADDR_IN *)buf);

				sockreq.packet_type = connect_req;
				sockreq.s = s;
				sockreq.sb = sb;
				sockreq.params.connect_s.buf = buf;
				sockreq.params.connect_s.namelen = namelen;

				TRIGGER_THREAD;

				if (sb->resultval)
				{
					if (sb->sb_errno == WSAEWOULDBLOCK-WSABASEERR)
					{
						if (sb->ftable[sd-1] & SF_BLOCKING)
						{
							seterrno(sb,0);

							WAITSIGNAL;

							if (sb->eintr)
							{
								// Destroy socket to cancel abort, replace it with fake socket to enable proper closing.
								// This is in accordance with BSD behaviour.
								win32_shutdown(s,1);
								win32_closesocket(s);
								sb->dtable[sd-1] = win32_socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
							}
						}
						else
						{
							seterrno(sb,36);	// EINPROGRESS
						}
					}
					else
					{
						CANCELSIGNAL; // Cancel pending signal
					}
				}

				ENDBLOCKING;

				if (sb->mtable[sd-1] == 0)
				{
					cancelasyncmsg(wMsg);
				}
				else
				{
					setWSAAsyncSelect(sb,sd,s,0);
				}
			}
		}
		else
			write_log ("BSDSOCK: WARNING - Excessive namelen (%d) in win32_connect()!\n",namelen);
#endif

	}

	TRACE(("%d\n",sb->sb_errno));
}

void host_sendto(TrapContext *context,SB, uae_u32 sd, uae_u32 msg, uae_u32 len, uae_u32 flags, uae_u32 to, uae_u32 tolen)
{
Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	SOCKET s;
	char *realpt;
	unsigned int wMsg;
	char buf[MAXADDRLEN];
	int iCut;
	HANDLE thread = GetCurrentThread();

#ifdef TRACING_ENABLED
	if (to)
	TRACE(("win32_sendto(%d,0x%lx,%d,0x%lx,0x%lx,%d) -> ",sd,msg,len,flags,to,tolen));
	else
	TRACE(("win32_send(%d,0x%lx,%d,%d) -> ",sd,msg,len,flags));
#endif
	sd++;
	s = getsock(sb,sd);

	if (s != INVALID_SOCKET)
	{
		realpt = get_real_address (msg);

		if (to)
		{
			if (tolen > sizeof buf) write_log ("BSDSOCK: WARNING - Target address in win32_sendto() too large (%d)!\n",tolen);
			else
			{
				memcpy(buf,get_real_address (to),tolen);
				// some Amiga software sets this field to bogus values
				prephostaddr((SOCKADDR_IN *)buf);
			}
		}
		if (sb->ftable[sd-1]&SF_RAW_RAW)
		{
			if (*(realpt+9) == 0x1)
			{ // ICMP
				struct sockaddr_in sin;
				win32_shutdown(s,1);
				win32_closesocket(s);
				s = win32_socket(AF_INET,SOCK_RAW,IPPROTO_ICMP);

				sin.sin_family = AF_INET;
				sin.sin_addr.s_addr = INADDR_ANY;
				sin.sin_port = (unsigned short) (*(realpt+21)&0xff)*256 + (unsigned short) (*(realpt+20)&0xff);
				win32_bind(s,(struct sockaddr *)&sin,sizeof(sin)) ;

				sb->dtable[sd-1] = s;
				sb->ftable[sd-1]&= ~SF_RAW_RAW;
				sb->ftable[sd-1]|= SF_RAW_RICMP;
			}
			if (*(realpt+9) == 0x11)
			{ // UDP
				struct sockaddr_in sin;
				win32_shutdown(s,1);
				win32_closesocket(s);
				s = win32_socket(AF_INET,SOCK_RAW,IPPROTO_UDP);

				sin.sin_family = AF_INET;
				sin.sin_addr.s_addr = INADDR_ANY;
				sin.sin_port = (unsigned short) (*(realpt+21)&0xff)*256 + (unsigned short) (*(realpt+20)&0xff);
				win32_bind(s,(struct sockaddr *)&sin,sizeof(sin)) ;

				sb->dtable[sd-1] = s;
				sb->ftable[sd-1]&= ~SF_RAW_RAW;
				sb->ftable[sd-1]|= SF_RAW_RUDP;
			}
		}

		BEGINBLOCKING;

		for (;;)
		{
			PREPARE_THREAD;

			sockreq.packet_type = sendto_req;
			sockreq.s = s;
			sockreq.sb = sb;
			sockreq.params.sendto_s.realpt = realpt;
			sockreq.params.sendto_s.buf = buf;
			sockreq.params.sendto_s.sd = sd;
			sockreq.params.sendto_s.msg = msg;
			sockreq.params.sendto_s.len = len;
			sockreq.params.sendto_s.flags = flags;
			sockreq.params.sendto_s.to = to;
			sockreq.params.sendto_s.tolen = tolen;

			if (sb->ftable[sd-1]&SF_RAW_UDP)
			{
				*(buf+2) = *(realpt+2);
				*(buf+3) = *(realpt+3);
				// Copy DST-Port
				iCut = 8;
				sockreq.params.sendto_s.realpt += iCut;
				sockreq.params.sendto_s.len -= iCut;
			}
			if (sb->ftable[sd-1]&SF_RAW_RUDP)
			{
				int iTTL;
				iTTL = (int) *(realpt+8)&0xff;
				win32_setsockopt(s,IPPROTO_IP,4,(char*) &iTTL,sizeof(iTTL));
				*(buf+2) = *(realpt+22);
				*(buf+3) = *(realpt+23);
				// Copy DST-Port
				iCut = 28;
				sockreq.params.sendto_s.realpt += iCut;
				sockreq.params.sendto_s.len -= iCut;
			}
			if (sb->ftable[sd-1]&SF_RAW_RICMP)
			{
				int iTTL;
				iTTL = (int) *(realpt+8)&0xff;
				win32_setsockopt(s,IPPROTO_IP,4,(char*) &iTTL,sizeof(iTTL));
				iCut = 20;
				sockreq.params.sendto_s.realpt += iCut;
				sockreq.params.sendto_s.len -= iCut;
			}

			TRIGGER_THREAD;

			if (sb->ftable[sd-1]&SF_RAW_UDP||sb->ftable[sd-1]&SF_RAW_RUDP||sb->ftable[sd-1]&SF_RAW_RICMP)
			{
				sb->resultval += iCut;
			}
			if (sb->resultval == -1)
			{
				if (sb->sb_errno != WSAEWOULDBLOCK-WSABASEERR || !(sb->ftable[sd-1] & SF_BLOCKING)) break;
			}
			else
			{
				realpt += sb->resultval;
				len -= sb->resultval;

				if (len <= 0) break;
				else continue;
			}

			if (sb->mtable[sd-1] || (wMsg = allocasyncmsg(sb,sd,s)) != 0)
			{
				if (sb->mtable[sd-1] == 0)
				{
//					WSAAsyncSelect(s,hWndSelector ? hAmigaWnd : hSockWnd,wMsg,FD_WRITE);
				}
				else
				{
					setWSAAsyncSelect(sb,sd,s,FD_WRITE);
				}

				WAITSIGNAL;

				if (sb->mtable[sd-1] == 0)
				{
					cancelasyncmsg(context, wMsg);	// context added, due to domino effect...
				}
				else
				{
					setWSAAsyncSelect(sb,sd,s,0);
				}

				if (sb->eintr)
				{
					TRACE(("[interrupted]\n"));
					return;
				}
			}
			else break;
		}

		ENDBLOCKING;
	}
	else sb->resultval = -1;

#ifdef TRACING_ENABLED
	if (sb->resultval == -1)
		TRACE(("failed (%d)\n",sb->sb_errno));
	else
		TRACE(("%d\n",sb->resultval));
#endif

}

void host_recvfrom(TrapContext *context,SB, uae_u32 sd, uae_u32 msg, uae_u32 len, uae_u32 flags, uae_u32 addr, uae_u32 addrlen)
{
Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	SOCKET s;
	char *realpt;
	struct sockaddr *rp_addr = NULL;
	unsigned int hlen;
	unsigned int wMsg;

#ifdef TRACING_ENABLED
	if (addr)
	TRACE(("win32_recvfrom(%d,0x%lx,%d,0x%lx,0x%lx,%d) -> ",sd,msg,len,flags,addr,get_long (addrlen)));
	else
	TRACE(("win32_recv(%d,0x%lx,%d,0x%lx) -> ",sd,msg,len,flags));
#endif
	sd++;
	s = getsock(sb,sd);

	if (s != INVALID_SOCKET)
	{
	realpt = get_real_address (msg);

	if (addr)
	{
		hlen = get_long (addrlen);
		rp_addr = (struct sockaddr *)get_real_address (addr);
	}

	BEGINBLOCKING;

	for (;;)
	{
		PREPARE_THREAD;

		sockreq.packet_type = recvfrom_req;
		sockreq.s = s;
		sockreq.sb = sb;
		sockreq.params.recvfrom_s.addr = addr;
		sockreq.params.recvfrom_s.flags = flags;
		sockreq.params.recvfrom_s.hlen = &hlen;
		sockreq.params.recvfrom_s.len = len;
		sockreq.params.recvfrom_s.realpt = realpt;
		sockreq.params.recvfrom_s.rp_addr = rp_addr;

		TRIGGER_THREAD;

		if (sb->resultval == -1)
		{
			if (sb->sb_errno == WSAEWOULDBLOCK-WSABASEERR && sb->ftable[sd-1] & SF_BLOCKING)
			{
				if (sb->mtable[sd-1] || (wMsg = allocasyncmsg(sb,sd,s)) != 0)
				{
					if (sb->mtable[sd-1] == 0)
					{
//						WSAAsyncSelect(s,hWndSelector ? hAmigaWnd : hSockWnd,wMsg,FD_READ|FD_CLOSE);
					}
					else
					{
						setWSAAsyncSelect(sb,sd,s,FD_READ|FD_CLOSE);
					}

					WAITSIGNAL;

					if (sb->mtable[sd-1] == 0)
					{
						cancelasyncmsg(context, wMsg); // added context, due to domino effect
					}
					else
					{
						setWSAAsyncSelect(sb,sd,s,0);
					}

					if (sb->eintr)
					{
						TRACE(("[interrupted]\n"));
						return;
					}
				}
				else break;
			}
			else break;
		}
		else break;
	}

	ENDBLOCKING;

	if (addr)
	{
		prepamigaaddr(rp_addr,hlen);
		put_long (addrlen,hlen);
	}
	}
	else sb->resultval = -1;

#ifdef TRACING_ENABLED
	if (sb->resultval == -1)
	TRACE(("failed (%d)\n",sb->sb_errno));
	else
	TRACE(("%d\n",sb->resultval));
#endif

}

uae_u32 host_shutdown(SB, uae_u32 sd, uae_u32 how)
{
Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	HANDLE thread = GetCurrentThread();
	SOCKET s;

	TRACE(("win32_shutdown(%d,%d) -> ",sd,how));
	sd++;
	s = getsock(sb,sd);

	if (s != INVALID_SOCKET)
	{
	if (win32_shutdown(s,how))
	{
		SETERRNO;
		TRACE(("failed (%d)\n",sb->sb_errno));
	}
	else
	{
		TRACE(("OK\n"));
		return 0;
	}
	}

	return -1;
}

void host_setsockopt(SB, uae_u32 sd, uae_u32 level, uae_u32 optname, uae_u32 optval, uae_u32 len)
{
Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	HANDLE thread = GetCurrentThread();
	SOCKET s;
	char buf[MAXADDRLEN];

	TRACE(("win32_setsockopt(%d,%d,0x%lx,0x%lx,%d) -> ",sd,(short)level,optname,optval,len));
	sd++;
	s = getsock(sb,sd);

	if (s != INVALID_SOCKET)
	{
	if (len > sizeof buf)
	{
		write_log ("BSDSOCK: WARNING - Excessive optlen in win32_setsockopt() (%d)\n",len);
		len = sizeof buf;
	}
	if (level == IPPROTO_IP && optname == 2)
		{ // IP_HDRINCL emulated by icmp.dll
		sb->resultval = 0;
		return;
		}
	if (level == SOL_SOCKET && optname == SO_LINGER)
	{
		((LINGER *)buf)->l_onoff = get_long (optval);
		((LINGER *)buf)->l_linger = get_long (optval+4);
	}
	else
	{
		if (len == 4) *(long *)buf = get_long (optval);
		else if (len == 2) *(short *)buf = get_word (optval);
		else write_log ("BSDSOCK: ERROR - Unknown optlen (%d) in win32_setsockopt(%d,%d)\n",level,optname);
	}

	// handle SO_EVENTMASK
	if (level == 0xffff && optname == 0x2001)
	{
		long wsbevents = 0;
		uae_u32 eventflags = get_long (optval);

		sb->ftable[sd-1] = (sb->ftable[sd-1] & ~REP_ALL) | (eventflags & REP_ALL);

		if (eventflags & REP_ACCEPT) wsbevents |= FD_ACCEPT;
		if (eventflags & REP_CONNECT) wsbevents |= FD_CONNECT;
		if (eventflags & REP_OOB) wsbevents |= FD_OOB;
		if (eventflags & REP_READ) wsbevents |= FD_READ;
		if (eventflags & REP_WRITE) wsbevents |= FD_WRITE;
		if (eventflags & REP_CLOSE) wsbevents |= FD_CLOSE;

		if (sb->mtable[sd-1] || (sb->mtable[sd-1] = allocasyncmsg(sb,sd,s)))
		{
//			WSAAsyncSelect(s,hWndSelector ? hAmigaWnd : hSockWnd,sb->mtable[sd-1],wsbevents);
			sb->resultval = 0;
		}
		else sb->resultval = -1;
	}
	else sb->resultval = win32_setsockopt(s,level,optname,buf,len);

	if (!sb->resultval)
	{
		TRACE(("OK\n"));
		return;
	}
	else SETERRNO;

	TRACE(("failed (%d)\n",sb->sb_errno));
	}
}

uae_u32 host_getsockopt(SB, uae_u32 sd, uae_u32 level, uae_u32 optname, uae_u32 optval, uae_u32 optlen)
{
Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	HANDLE thread = GetCurrentThread();
	SOCKET s;
	char buf[MAXADDRLEN];
	unsigned int len = sizeof(buf);

	TRACE(("win32_getsockopt(%d,%d,0x%lx,0x%lx,0x%lx) -> ",sd,(short)level,optname,optval,optlen));
	sd++;
	s = getsock(sb,sd);

	if (s != INVALID_SOCKET)
	{
		if (!win32_getsockopt(s,level,optname,buf,&len))
		{
			if (level == SOL_SOCKET && optname == SO_LINGER)
			{
				put_long (optval,((LINGER *)buf)->l_onoff);
				put_long (optval+4,((LINGER *)buf)->l_linger);
			}
			else
			{
				if (len == 4) put_long (optval,*(long *)buf);
				else if (len == 2) put_word (optval,*(short *)buf);
				else write_log ("BSDSOCK: ERROR - Unknown optlen (%d) in win32_setsockopt(%d,%d)\n",level,optname);
			}

//			put_long (optlen,len); // some programs pass the actual ength instead of a pointer to the length, so...
			TRACE(("OK (%d,%d)\n",len,*(long *)buf));
			return 0;
		}
		else
		{
			SETERRNO;
			TRACE(("failed (%d)\n",sb->sb_errno));
		}
	}

	return -1;
}

uae_u32 host_getsockname(SB, uae_u32 sd, uae_u32 name, uae_u32 namelen)
{
Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	SOCKET s;
	unsigned int len;
	struct sockaddr *rp_name;

	sd++;
	len = get_long (namelen);

	TRACE(("getsockname(%d,0x%lx,%d) -> ",sd,name,len));

	s = getsock(sb,sd);

	if (s != INVALID_SOCKET)
	{
		rp_name = (struct sockaddr *)get_real_address (name);

		if (getsockname(s,rp_name,&len))
		{
			SETERRNO;
			TRACE(("failed (%d)\n",sb->sb_errno));
		}
		else
		{
			TRACE(("%d\n",len));
			prepamigaaddr(rp_name,len);
			put_long (namelen,len);
			return 0;
		}
	}

	return -1;
}

uae_u32 host_getpeername(SB, uae_u32 sd, uae_u32 name, uae_u32 namelen)
{
Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	SOCKET s;
	unsigned int len;
	struct sockaddr *rp_name;

	sd++;
	len = get_long (namelen);

	TRACE(("getpeername(%d,0x%lx,%d) -> ",sd,name,len));

	s = getsock(sb,sd);

	if (s != INVALID_SOCKET)
	{
		rp_name = (struct sockaddr *)get_real_address (name);

		if (getpeername(s,rp_name,&len))
		{
			SETERRNO;
			TRACE(("failed (%d)\n",sb->sb_errno));
		}
		else
		{
			TRACE(("%d\n",len));
			prepamigaaddr(rp_name,len);
			put_long (namelen,len);
			return 0;
		}
	}

	return -1;
}

uae_u32 host_IoctlSocket(SB, uae_u32 sd, uae_u32 request, uae_u32 arg)
{
Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	HANDLE thread = GetCurrentThread();
	SOCKET s;
	uae_u32 data;
	int success = SOCKET_ERROR;

	TRACE(("IoctlSocket(%d,0x%lx,0x%lx) ",sd,request,arg));
	sd++;
	s = getsock(sb,sd);

	if (s != INVALID_SOCKET)
	{
		switch (request)
		{
			case FIOSETOWN:
				sb->ownertask = get_long (arg);
				success = 0;
				break;

			case FIOGETOWN:
				put_long (arg,sb->ownertask);
				success = 0;
				break;

			case FIONBIO:
				TRACE(("[FIONBIO] -> "));
				if (get_long (arg))
				{
					TRACE(("nonblocking\n"));
					sb->ftable[sd-1] &= ~SF_BLOCKING;
				}
				else
				{
					TRACE(("blocking\n"));
					sb->ftable[sd-1] |= SF_BLOCKING;
				}
				success = 0;
				break;

			case FIONREAD:
				win32_IoctlSocket(s,request,(u_long *)&data);
				TRACE(("[FIONREAD] -> %d\n",data));
				put_long (arg,data);
				success = 0;
				break;

			case FIOASYNC:
				if (get_long (arg))
				{
					sb->ftable[sd-1] |= REP_ALL;

					TRACE(("[FIOASYNC] -> enabled\n"));
					if (sb->mtable[sd-1] || (sb->mtable[sd-1] = allocasyncmsg(sb,sd,s)))
					{
//						WSAAsyncSelect(s,hWndSelector ? hAmigaWnd : hSockWnd,sb->mtable[sd-1],FD_ACCEPT | FD_CONNECT | FD_OOB | FD_READ | FD_WRITE | FD_CLOSE);
						success = 0;
						break;
					}
				}
				else write_log (("BSDSOCK: WARNING - FIOASYNC disabling unsupported.\n"));

				success = -1;
				break;

			default:
				write_log ("BSDSOCK: WARNING - Unknown IoctlSocket request: 0x%08lx\n",request);
				seterrno(sb,22);	// EINVAL
		}
	}

	return success;
}

int host_CloseSocket( SB, int sd)
{
Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	HANDLE thread = GetCurrentThread();
	unsigned int wMsg;
	SOCKET s;

	struct TrapContext *context = NULL;	// no idea....

	TRACE(("CloseSocket(%d) -> ",sd));
	sd++;

	s = getsock(sb,sd);
	if (s != INVALID_SOCKET)
	{
		if (sb->mtable[sd-1])
		{
			asyncsb[(sb->mtable[sd-1]-0xb000)/2] = NULL;
			sb->mtable[sd-1] = 0;
		}

		if (checksd(sb ,sd) == TRUE)
			return 0;

		BEGINBLOCKING;

		for (;;)
		{
			win32_shutdown(s,1);

			if (!win32_closesocket(s))
			{
				releasesock(sb,sd);
				TRACE(("OK\n"));
				return 0;
			}

			SETERRNO;

			if (sb->sb_errno != WSAEWOULDBLOCK-WSABASEERR || !(sb->ftable[sd-1] & SF_BLOCKING)) break;

			if ((wMsg = allocasyncmsg(sb,sd,s)) != 0)
			{
//				WSAAsyncSelect(s,hWndSelector ? hAmigaWnd : hSockWnd,wMsg,FD_CLOSE);

				WAITSIGNAL;	// needs, context... so host_CloseSocke, needs context too..

				cancelasyncmsg(context, wMsg);

				if (sb->eintr)
				{
					TRACE(("[interrupted]\n"));
					break;
				}
			}
			else break;
		}

		ENDBLOCKING;
	}

	TRACE(("failed (%d)\n",sb->sb_errno));

	return -1;
}

// For the sake of efficiency, we do not malloc() the fd_sets here.
// 64 sockets should be enough for everyone.
static void makesocktable(SB, uae_u32 fd_set_amiga, struct fd_set *fd_set_win, int nfds, SOCKET addthis)
{
Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	int i, j;
	uae_u32 currlong, mask;
	SOCKET s;

	if (addthis != INVALID_SOCKET)
	{
		*fd_set_win->fd_array = addthis;
		fd_set_win->fd_count = 1;
	}
	else fd_set_win->fd_count = 0;

	if (!fd_set_amiga)
	{
		fd_set_win->fd_array[fd_set_win->fd_count] = INVALID_SOCKET;
		return;
	}

	if (nfds > sb->dtablesize)
	{
		write_log ("BSDSOCK: ERROR - win32_select()ing more sockets (%d) than socket descriptors available (%d)!\n",nfds,sb->dtablesize);
		nfds = sb->dtablesize;
	}

	for (j = 0; ; j += 32, fd_set_amiga += 4)
	{
		currlong = get_long (fd_set_amiga);

		mask = 1;

		for (i = 0; i < 32; i++, mask <<= 1)
		{
			if (i+j > nfds)
			{
				fd_set_win->fd_array[fd_set_win->fd_count] = INVALID_SOCKET;
				return;
			}

			if (currlong & mask)
			{
				s = getsock(sb,j+i+1);

				if (s != INVALID_SOCKET)
				{
					fd_set_win->fd_array[fd_set_win->fd_count++] = s;

					if (fd_set_win->fd_count >= FD_SETSIZE)
					{
						write_log ("BSDSOCK: ERROR - win32_select()ing more sockets (%d) than the hard-coded fd_set limit (%d) - please report\n",nfds,FD_SETSIZE);
						return;
					}
				}
			}
		}
	}

	fd_set_win->fd_array[fd_set_win->fd_count] = INVALID_SOCKET;
}

static void makesockbitfield(SB, uae_u32 fd_set_amiga, struct fd_set *fd_set_win, int nfds)
{
Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	int n, i, j, val, mask;
	SOCKET currsock;

	for (n = 0; n < nfds; n += 32)
	{
		val = 0;
		mask = 1;

		for (i = 0; i < 32; i++, mask <<= 1)
		{
			if ((currsock = getsock(sb, n+i+1)) != INVALID_SOCKET)
			{ // Do not use sb->dtable directly because of Newsrog
				for (j = fd_set_win->fd_count; j--; )
				{
					if (fd_set_win->fd_array[j] == currsock)
					{
						val |= mask;
						break;
					}
				}
			}
		}
		put_long (fd_set_amiga,val);
		fd_set_amiga += 4;
	}
}

static void fd_zero(uae_u32 fdset, uae_u32 nfds)
{
Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	unsigned int i;
	for (i = 0; i < nfds; i += 32, fdset += 4) put_long (fdset,0);
}


#undef fd_set


// Helper to convert custom FD_SET to system fd_set
static void convert_fdset_to_native( struct _FD_SET *custom, fd_set *native)
{
Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	FD_ZERO(native);
	if ((!custom) || (!native)) return;
	for (int i = 0; i < custom->fd_count; ++i)
	{
		FD_SET(custom->fd_array[i], native);
		native++; // next
	}
}

#define sf(expose,ptr) if (ptr) expose(ptr); ptr = NULL;


int win32_select_wrapper(
					long nfds,
					struct _FD_SET *readsocks,
					struct _FD_SET *writesocks,
					struct _FD_SET *exceptsocks,
					struct timeval *timeout
				)
{
	HANDLE thread = GetCurrentThread();
	int resultval = 0;

#if 1

	fd_set *native_read = NULL, *native_write = NULL, *native_except = NULL;

	if (readsocks)
	{
		native_read = (fd_set *) malloc(sizeof(fd_set) * nfds);
		convert_fdset_to_native(readsocks, native_read);
	}

	if (writesocks)
	{
		native_write = (fd_set *) malloc(sizeof(fd_set) * nfds);
		convert_fdset_to_native(writesocks, native_write);
	}

	if (exceptsocks)
	{
		native_except = (fd_set *) malloc(sizeof(fd_set) * nfds);
		convert_fdset_to_native(exceptsocks, native_except);
	}

	resultval =  native_WaitSelect(nfds, native_read, native_write, native_except, timeout, NULL);

	sf(free,native_read);
	sf(free,native_write);
   	sf(free,native_except);

#endif

	return resultval;
}


#define fd_set _FD_SET
#define select win32_select_wrapper

#if 1

// This seems to be the only way of implementing a cancelable WinSock2 win32_select() call... sigh.
static unsigned int __stdcall thread_WaitSelect(void *index2)
{
	uae_u32 index = (uae_u32)index2;
	unsigned int result = 0;
	long nfds;
	uae_u32 readfds, writefds, exceptfds;
	uae_u32 timeout;
	struct fd_set readsocks, writesocks, exceptsocks;
	struct timeval tv;
	uae_u32 *args;

	SB;

	for (;;)
	{
		WaitForSingleObject(hEvents[index],INFINITE);

		if ((args = threadargs[index]) != NULL)
		{
			sb = (struct socketbase *)*args;
			nfds = args[1];
			readfds = args[2];
			writefds = args[3];
			exceptfds = args[4];
			timeout = args[5];

			// construct descriptor tables
			makesocktable(sb,readfds,&readsocks,nfds,sb->sockAbort);
			if (writefds) makesocktable(sb,writefds,&writesocks,nfds,INVALID_SOCKET);
			if (exceptfds) makesocktable(sb,exceptfds,&exceptsocks,nfds,INVALID_SOCKET);

			if (timeout)
			{
				tv.tv_sec = get_long (timeout);
				tv.tv_usec = get_long (timeout+4);
				TRACE(("(timeout: %d.%06d) ",tv.tv_sec,tv.tv_usec));
			}

			TRACE(("-> "));

			sb->resultval = win32_select_wrapper(
					nfds+1,
					&readsocks,
					writefds ? &writesocks : NULL,
					exceptfds ? &exceptsocks : NULL,
					timeout ? &tv : 0
					);

			if (sb->resultval == SOCKET_ERROR)
			{
				// select was stopped by sb->sockAbort
				if (readsocks.fd_count > 1)
				{
					makesocktable(sb,readfds,&readsocks,nfds,INVALID_SOCKET);
					tv.tv_sec = 0;
					tv.tv_usec = 10000;
					// Check for 10ms if data is available
					sb->resultval = select(nfds+1,&readsocks,writefds ? &writesocks : NULL,exceptfds ? &exceptsocks : NULL,&tv);
					if (sb->resultval == 0)
					{ // Now timeout -> really no data available
						if (GetLastError() != 0)
						{
							sb->resultval = SOCKET_ERROR;
							// Set old resultval
						}
					}
				}
			}

			if (FD_ISSET(sb->sockAbort,&readsocks))
			{
				if (sb->resultval != SOCKET_ERROR)
				{
					sb->resultval--;
				}
			}
			else
			{
				sb->needAbort = 0;
			}

			if (sb->resultval == SOCKET_ERROR)
			{
				SETERRNO;
				TRACE(("failed (%d) - ",sb->sb_errno));
				if (readfds) fd_zero(readfds,nfds);
				if (writefds) fd_zero(writefds,nfds);
				if (exceptfds) fd_zero(exceptfds,nfds);
			}
			else
			{
				if (readfds) makesockbitfield(sb,readfds,&readsocks,nfds);
				if (writefds) makesockbitfield(sb,writefds,&writesocks,nfds);
				if (exceptfds) makesockbitfield(sb,exceptfds,&exceptsocks,nfds);
			}

			SETSIGNAL;

			threadargs[index] = NULL;
			SetEvent(sb->hEvent);
		}
	}
#ifndef __GNUC__
	_endthreadex( result );
#endif
	return result;
}

#endif

void ResetEvent( HANDLE hEvent )
{
	struct Message *msg;

	// clear the message queue by reply all messages... don't know if this what it should do!!

	if (hEvent -> type == h_event)
	{
		if ((hEvent && hEvent -> object))
		{
			struct MsgPort *msgport = (struct MsgPort *) hEvent -> object;
			while (msg = GetMsg(msgport))
			{
				ReplyMsg( msg );
			}
		}
		else
		{
			printf("%s:%d -- object not found\n");
		}
	}
	else
	{
		printf("%s:%d -- wrong type of object\n");
	}
}

void bsdlib_reset()
{
	Printf("%s:%s:%ld -- not yet implmented\n",__FILE__,__FUNCTION__,__LINE__);

	// no idea what to do here!!
	// but this function should, clear all data, and init all data...
}

void host_WaitSelect(TrapContext *context, SB, uae_u32 nfds, uae_u32 readfds, uae_u32 writefds, uae_u32 exceptfds, uae_u32 timeout, uae_u32 sigmp)
{
	HANDLE thread = GetCurrentThread();

	uae_u32 sigs, wssigs;
	int i;

	wssigs = sigmp ? get_long (sigmp) : 0;

	TRACE(("WaitSelect(%d,0x%lx,0x%lx,0x%lx,0x%lx,0x%lx) ",nfds,readfds,writefds,exceptfds,timeout,wssigs));

	if (!readfds && !writefds && !exceptfds && !timeout && !wssigs)
	{
		sb->resultval = 0;
		TRACE(("-> [ignored]\n"));
		return;
	}

	if (wssigs)
	{
		m68k_dreg (&regs, 0) = 0;
		m68k_dreg (&regs, 1) = wssigs;
		sigs = CallLib (get_long (4),-0x132) & wssigs;	// SetSignal()

		if (sigs)
		{
			TRACE(("-> [preempted by signals 0x%08lx]\n",sigs & wssigs));
			put_long (sigmp,sigs & wssigs);
			// Check for zero address -> otherwise WinUAE crashes
			if (readfds) fd_zero(readfds,nfds);
			if (writefds) fd_zero(writefds,nfds);
			if (exceptfds) fd_zero(exceptfds,nfds);
			sb->resultval = 0;
			seterrno(sb,0);
			return;
		}
	}

	if (nfds == 0)
	{ // No sockets to check, only wait for signals
		m68k_dreg (&regs, 0) = wssigs;
		sigs = CallLib (get_long (4),-0x13e);	// Wait()

		put_long (sigmp,sigs & wssigs);

		if (readfds) fd_zero(readfds,nfds);
		if (writefds) fd_zero(writefds,nfds);
		if (exceptfds) fd_zero(exceptfds,nfds);
		sb->resultval = 0;
		return;
	}

	ResetEvent(sb->hEvent);

	sb->needAbort = 1;

	for (i = 0; i < MAX_SELECT_THREADS; i++) if (hThreads[i] && !threadargs[i]) break;

	if (i >= MAX_SELECT_THREADS)
	{
		for (i = 0; i < MAX_SELECT_THREADS; i++)
		{
			if (!hThreads[i])
			{
				if ((hEvents[i] = CreateEvent(NULL,FALSE,FALSE,NULL)) == NULL || (hThreads[i] = (void *)THREAD(thread_WaitSelect,i)) == NULL)
				{
					hThreads[i] = 0;
					write_log ("BSDSOCK: ERROR - Thread/Event creation failed - error code: %d\n",GetLastError());
					seterrno(sb,12);	// ENOMEM
					sb->resultval = -1;
					return;
				}

				// this should improve responsiveness
				SetThreadPriority(hThreads[i],THREAD_PRIORITY_TIME_CRITICAL);
				break;
			}
		}
	}

	if (i >= MAX_SELECT_THREADS) write_log ("BSDSOCK: ERROR - Too many win32_select()s\n");
	else
	{
		SOCKET newsock = INVALID_SOCKET;

		threadargs[i] = (uae_u32 *)&sb;

		SetEvent(hEvents[i]);

		m68k_dreg (&regs, 0) = (((uae_u32)1)<<sb->signal)|sb->eintrsigs|wssigs;
		sigs = CallLib (get_long (4),-0x13e);	// Wait()
/*
		if ((1<<sb->signal) & sigs)
		{ // 2.3.2002/SR Fix for AmiFTP -> Thread is ready, no need to Abort
			sb->needAbort = 0;
		}
*/
		if (sb->needAbort)
		{
			if ((newsock = win32_socket(AF_INET,SOCK_STREAM,IPPROTO_TCP)) == INVALID_SOCKET)
				write_log ("BSDSOCK: ERROR - Cannot create socket: %d\n",WSAGetLastError());

			win32_shutdown(sb->sockAbort,1);
			if (newsock != sb->sockAbort)
			{
				win32_shutdown(sb->sockAbort,1);
				win32_closesocket(sb->sockAbort);
			}
		}

		WaitForSingleObject(sb->hEvent,INFINITE);

		CANCELSIGNAL;

		if (newsock != INVALID_SOCKET) sb->sockAbort = newsock;

		if( sigmp )
		{
			put_long (sigmp,sigs & wssigs);

			if (sigs & sb->eintrsigs)
			{
				TRACE(("[interrupted]\n"));
				sb->resultval = -1;
				seterrno(sb,4);	// EINTR
			}
			else if (sigs & wssigs)
			{
				TRACE(("[interrupted by signals 0x%08lx]\n",sigs & wssigs));

				if (readfds) fd_zero(readfds,nfds);
				if (writefds) fd_zero(writefds,nfds);
				if (exceptfds) fd_zero(exceptfds,nfds);

				seterrno(sb,0);
				sb->resultval = 0;
			}

			if (sb->resultval >= 0)
			{
				TRACE(("%d\n",sb->resultval));
			}
			else
			{
				TRACE(("%d errno %d\n",sb->resultval,sb->sb_errno));
			}
		}
		else TRACE(("%d\n",sb->resultval));
	}
}

uae_u32 host_Inet_NtoA(TrapContext *context, SB, uae_u32 in)
{
	char *addr;
	struct in_addr ina;
	uae_u32 scratchbuf;

	*(uae_u32 *)&ina = htonl(in);

	TRACE(("Inet_NtoA(%lx) -> ",in));

	if ((addr = inet_ntoa(ina)) != NULL)
	{
		scratchbuf = m68k_areg (&regs, 6)+offsetof(struct UAEBSDBase,scratchbuf);
		strncpyha(scratchbuf,addr,SCRATCHBUFSIZE);
		TRACE(("%s\n",addr));
		return scratchbuf;
	}
	else SETERRNO;

	TRACE(("failed (%d)\n",sb->sb_errno));

	return 0;
}

uae_u32 host_inet_addr(uae_u32 cp)
{
	uae_u32 addr;
	char *cp_rp;

	cp_rp = get_real_address (cp);

	addr = htonl(inet_addr(cp_rp));

	TRACE(("inet_addr(%s) -> 0x%08lx\n",cp_rp,addr));

	return addr;
}

int isfullscreen (void);

BOOL CheckOnline(SB)
{
	DWORD dwFlags;
	BOOL bReturn = TRUE;

#if 0

	if (InternetGetConnectedState(&dwFlags,0) == FALSE)
	{ // Internet is offline

		if (InternetAttemptConnect(0) != ERROR_SUCCESS)
		{ // Show Dialer window
			sb->sb_errno = 10001;
			sb->sb_herrno = 1;
			bReturn = FALSE;
			// No success or aborted
		}


		if (isfullscreen())
		{
			ShowWindow (hAmigaWnd, SW_RESTORE);
			SetActiveWindow(hAmigaWnd);
		}
	}
#else

	printf("function %s() not expected\n",__FUNCTION__);

#endif

	return(bReturn);
}

static unsigned int __stdcall thread_get(void *index2)
{
	uae_u32 index = (uae_u32)index2;
	unsigned int result = 0;
	uae_u32 *args;
	uae_u32 name;
	uae_u32 namelen;
	long addrtype;
	char *name_rp;
	char *buf;

	SB;

	for (;;)
	{
		WaitForSingleObject(hGetEvents[index],INFINITE);

		if (threadGetargs[index] ==  (void *) -1)
		{
			threadGetargs[index] = NULL;
		}

		if ((args = threadGetargs[index]) != NULL )
		{
			sb = (struct socketbase *)*args;
			if (args[1] == 0)
			{ // gethostbyname or gethostbyaddr
				struct hostent *host;
				name = args[2];
				namelen = args[3];
				addrtype = args[4];
				buf = (char*) args[5];
				name_rp = get_real_address (name);

				if (strchr(name_rp,'.') == 0 || CheckOnline(sb) == TRUE)
				{ // Local Address or Internet Online ?
					if (addrtype == -1)
					{
						host = gethostbyname(name_rp);
					}
					else
					{
						host = gethostbyaddr(name_rp,namelen,addrtype);
					}
					if (threadGetargs[index] != (void *) -1)
					{ // No CTRL-C Signal

						if (host == 0)
						{
							// Error occured
							SETERRNO;
							TRACE(("failed (%d) - ",sb->sb_errno));
						}
						else
						{
							seterrno(sb,0);
							memcpy(buf,host,sizeof(HOSTENT));
						}
					}
				}
			}

			if (args[1] == 1)
			{ // getprotobyname
				struct protoent  *proto;

				name = args[2];
				buf = (char*) args[5];
				name_rp = get_real_address (name);
				proto = getprotobyname (name_rp);
				if (threadGetargs[index] != (void *) -1)
				{ // No CTRL-C Signal

					if (proto == 0)
					{
						// Error occured
						SETERRNO;
						TRACE(("failed (%d) - ",sb->sb_errno));
					}
					else
					{
						seterrno(sb,0);
						memcpy(buf,proto,sizeof(struct protoent));
					}
				}
			}

			if (args[1] == 2)
			{ // getservbyport and getservbyname
				uae_u32 nameport;
				uae_u32 proto;
				uae_u32 type;
				char *proto_rp = 0;
				struct servent *serv;

				nameport = args[2];
				proto = args[3];
				type = args[4];
				buf = (char*) args[5];

				if (proto) proto_rp = get_real_address (proto);

				if (type)
				{
					serv = getservbyport(nameport,proto_rp);
				}
				else
				{
					name_rp = get_real_address (nameport);
					serv = getservbyname(name_rp,proto_rp);
				}

				if (threadGetargs[index] != (void *) -1)
				{ // No CTRL-C Signal
					if (serv == 0)
					{
						// Error occured
						SETERRNO;
						TRACE(("failed (%d) - ",sb->sb_errno));
					}
					else
					{
						seterrno(sb,0);
						memcpy(buf,serv,sizeof(struct servent));
					}
				}
			}

			TRACE(("-> "));

			if (threadGetargs[index] != (void *) -1)
				SETSIGNAL;

			threadGetargs[index] = NULL;
		}
	}

#ifndef __GNUC__
	_endthreadex( result );
#endif
	return result;
}


void host_gethostbynameaddr(TrapContext *context,SB, uae_u32 name, uae_u32 namelen, long addrtype)
{
	HOSTENT *h;
	int size, numaliases = 0, numaddr = 0;
	uae_u32 aptr;
	char *name_rp;
	int i;

	uae_u32 args[6];

	uae_u32 addr;
	uae_u32 *addr_list[2];

	char buf[MAXGETHOSTSTRUCT];
	unsigned int wMsg = 0;


//	char on = 1;
//	InternetSetOption(0,INTERNET_OPTION_SETTINGS_CHANGED,&on,strlen(&on));
//  Do not use: Causes locks with some machines

	name_rp = get_real_address (name);

	if (addrtype == -1)
	{
		TRACE(("gethostbyname(%s) -> ",name_rp));

		// workaround for numeric host "names"
		if ((addr = inet_addr(name_rp)) != INADDR_NONE)
		{
			seterrno(sb,0);
			((HOSTENT *)buf)->h_name = name_rp;
			((HOSTENT *)buf)->h_aliases = NULL;
			((HOSTENT *)buf)->h_addrtype = AF_INET;
			((HOSTENT *)buf)->h_length = 4;
			((HOSTENT *)buf)->h_addr_list = (char **)&addr_list;
			addr_list[0] = &addr;
			addr_list[1] = NULL;

			goto kludge;
		}
	}
	else
	{
		TRACE(("gethostbyaddr(0x%lx,0x%lx,%ld) -> ",name,namelen,addrtype));
	}

	args[0] = (uae_u32) sb;
	args[1] = 0;
	args[2] = name;
	args[3] = namelen;
	args[4] = addrtype;
	args[5] = (uae_u32) &buf[0];

	for (i = 0; i < MAX_GET_THREADS; i++)
		{
		if (threadGetargs[i] ==  (void *) -1)
			{
			threadGetargs[i] = 0;
			}
		if (hGetThreads[i] && !threadGetargs[i]) break;
		}

	if (i >= MAX_GET_THREADS)
	{
		for (i = 0; i < MAX_GET_THREADS; i++)
		{
			if (!hGetThreads[i])
			{
				if ((hGetEvents[i] = CreateEvent(NULL,FALSE,FALSE,NULL)) == NULL || (hGetThreads[i] = (void *)THREAD(thread_get,i)) == NULL)
				{
					hGetThreads[i] = 0;
					write_log ("BSDSOCK: ERROR - Thread/Event creation failed - error code: %d\n",GetLastError());
					seterrno(sb,12);	// ENOMEM
					sb->resultval = -1;
					return;
				}
				break;
			}
		}
	}

	if (i >= MAX_GET_THREADS) write_log ("BSDSOCK: ERROR - Too many gethostbyname()s\n");
	else
	{
		bsdsetpriority (hGetThreads[i]);
		threadGetargs[i] = (uae_u32 *)&args[0];

		SetEvent(hGetEvents[i]);
	}
	sb->eintr = 0;
	while ( threadGetargs[i] != 0 && sb->eintr == 0)
	{
		WAITSIGNAL;
		if (sb->eintr == 1)
			threadGetargs[i] =  (void *) -1;
	}

	CANCELSIGNAL;

	if (!sb->sb_errno)
	{
kludge:
		h = (HOSTENT *)buf;

		// compute total size of hostent
		size = 28;
		if (h->h_name != NULL) size += strlen(h->h_name)+1;

		if (h->h_aliases != NULL)
			while (h->h_aliases[numaliases]) size += strlen(h->h_aliases[numaliases++])+5;

		if (h->h_addr_list != NULL)
		{
			while (h->h_addr_list[numaddr]) numaddr++;
			size += numaddr*(h->h_length+4);
		}

		if (sb->hostent)
		{
			uae_FreeMem( context, sb->hostent, sb->hostentsize );
		}

		sb->hostent = uae_AllocMem( context, size, 0 );

		if (!sb->hostent)
		{
			write_log ("BSDSOCK: WARNING - gethostby%s() ran out of Amiga memory (couldn't allocate %ld bytes) while returning result of lookup for '%s'\n",addrtype == -1 ? "name" : "addr",size,(char *)name);
			seterrno(sb,12); // ENOMEM
			return;
		}

		sb->hostentsize = size;

		aptr = sb->hostent+28+numaliases*4+numaddr*4;

		// transfer hostent to Amiga memory
		put_long (sb->hostent+4,sb->hostent+20);
		put_long (sb->hostent+8,h->h_addrtype);
		put_long (sb->hostent+12,h->h_length);
		put_long (sb->hostent+16,sb->hostent+24+numaliases*4);

		for (i = 0; i < numaliases; i++) put_long (sb->hostent+20+i*4,addstr(&aptr,h->h_aliases[i]));
		put_long (sb->hostent+20+numaliases*4,0);
		for (i = 0; i < numaddr; i++) put_long (sb->hostent+24+(numaliases+i)*4,addmem(&aptr,h->h_addr_list[i],h->h_length));
		put_long (sb->hostent+24+numaliases*4+numaddr*4,0);
		put_long (sb->hostent,aptr);
		addstr(&aptr,h->h_name);

		TRACE(("OK (%s)\n",h->h_name));
		seterrno(sb,0);
	}
	else
	{
		TRACE(("failed (%d/%d)\n",sb->sb_errno,sb->sb_herrno));
	}

}

void host_getprotobyname(TrapContext *context, SB, uae_u32 name)
{
	PROTOENT *p;
	int size, numaliases = 0;
	uae_u32 aptr;
	char *name_rp;
	int i;

	uae_u32 args[6];

	char buf[MAXGETHOSTSTRUCT];

	name_rp = get_real_address (name);

	TRACE(("getprotobyname(%s) -> ",name_rp));

	args[0] = (uae_u32) sb;
	args[1] = 1;
	args[2] = name;
	args[5] = (uae_u32) &buf[0];

	for (i = 0; i < MAX_GET_THREADS; i++)
	{
		if (threadGetargs[i] == (void *) -1)
		{
			threadGetargs[i] = 0;
		}
		if (hGetThreads[i] && !threadGetargs[i]) break;
	}

	if (i >= MAX_GET_THREADS)
	{
		for (i = 0; i < MAX_GET_THREADS; i++)
		{
			if (!hGetThreads[i])
			{
				if ((hGetEvents[i] = CreateEvent(NULL,FALSE,FALSE,NULL)) == NULL || (hGetThreads[i] = (void *)THREAD(thread_get,i)) == NULL)
				{
					hGetThreads[i] = 0;
					write_log ("BSDSOCK: ERROR - Thread/Event creation failed - error code: %d\n",GetLastError());
					seterrno(sb,12);	// ENOMEM
					sb->resultval = -1;
					return;
				}
				break;
			}
		}
	}

	if (i >= MAX_GET_THREADS) write_log ("BSDSOCK: ERROR - Too many getprotobyname()s\n");
	else
	{
		bsdsetpriority (hGetThreads[i]);

		threadGetargs[i] = (uae_u32 *)&args[0];

		SetEvent(hGetEvents[i]);
	}

	sb->eintr = 0;
	while ( threadGetargs[i] != 0 && sb->eintr == 0)
	{
		WAITSIGNAL;
		if (sb->eintr == 1)
			threadGetargs[i] =  (void *) -1;
	}

	CANCELSIGNAL;


	if (!sb->sb_errno)
	{
		p = (PROTOENT *)buf;

		// compute total size of protoent
		size = 16;
		if (p->p_name != NULL) size += strlen(p->p_name)+1;

		if (p->p_aliases != NULL)
			while (p->p_aliases[numaliases]) size += strlen(p->p_aliases[numaliases++])+5;

		if (sb->protoent)
		{
			uae_FreeMem( context, sb->protoent, sb->protoentsize );
		}

		sb->protoent = uae_AllocMem( context, size, 0 );

		if (!sb->protoent)
		{
			write_log ("BSDSOCK: WARNING - getprotobyname() ran out of Amiga memory (couldn't allocate %ld bytes) while returning result of lookup for '%s'\n",size,(char *)name);
			seterrno(sb,12); // ENOMEM
			return;
		}

		sb->protoentsize = size;

		aptr = sb->protoent+16+numaliases*4;

		// transfer protoent to Amiga memory
		put_long (sb->protoent+4,sb->protoent+12);
		put_long (sb->protoent+8,p->p_proto);

		for (i = 0; i < numaliases; i++) put_long (sb->protoent+12+i*4,addstr(&aptr,p->p_aliases[i]));
		put_long (sb->protoent+12+numaliases*4,0);
		put_long (sb->protoent,aptr);
		addstr(&aptr,p->p_name);
		TRACE(("OK (%s, %d)\n",p->p_name,p->p_proto));
		seterrno(sb,0);
	}
	else
	{
		TRACE(("failed (%d)\n",sb->sb_errno));
	}

}

/*
 * Copy a protoent object from native space to Amiga space
 */
static void copyProtoent (TrapContext *context, SB, const struct protoent *p)
{
	size_t size = 16;
	int numaliases = 0;
	int i;
	uae_u32 aptr;

	// compute total size of protoent
	if (p->p_name != NULL)
	size += strlen (p->p_name) + 1;

	if (p->p_aliases != NULL)
	while (p->p_aliases[numaliases])
		size += strlen (p->p_aliases[numaliases++]) + 5;

	if (sb->protoent) {
	uae_FreeMem (context, sb->protoent, sb->protoentsize);
	}

	sb->protoent = uae_AllocMem (context, size, 0);

	if (!sb->protoent) {
	write_log ("BSDSOCK: WARNING - copyProtoent() ran out of Amiga memory (couldn't allocate %d bytes)\n", size);
	bsdsocklib_seterrno (sb, 12); // ENOMEM
	return;
	}

	sb->protoentsize = size;

	aptr = sb->protoent + 16 + numaliases * 4;

	// transfer protoent to Amiga memory
	put_long (sb->protoent + 4, sb->protoent + 12);
	put_long (sb->protoent + 8, p->p_proto);

	for (i = 0; i < numaliases; i++)
		put_long (sb->protoent + 12 + i * 4, addstr (&aptr, p->p_aliases[i]));

	put_long (sb->protoent + 12 + numaliases * 4, 0);
	put_long (sb->protoent, aptr);

	addstr (&aptr, p->p_name);
	bsdsocklib_seterrno(sb, 0);
}

void host_getprotobynumber (TrapContext *context, SB, uae_u32 number)
{
	struct protoent *p = getprotobynumber(number);
	write_log("getprotobynumber(%d)=%lx\n", number, p);

	if (p == NULL)
	{
		SETERRNO;
		return;
	}

	copyProtoent (context, sb, p);
	TRACE (("OK (%s, %d)\n", p->p_name, p->p_proto));
}

void host_getservbynameport(TrapContext *context, SB, uae_u32 nameport, uae_u32 proto, uae_u32 type)
{
	SERVENT *s;
	int size, numaliases = 0;
	uae_u32 aptr;
	char *name_rp = NULL, *proto_rp = NULL;
	int i;

	char buf[MAXGETHOSTSTRUCT];
	uae_u32 args[6];

	if (proto) proto_rp = get_real_address (proto);

	if (type)
	{
		TRACE(("getservbyport(%d,%s) -> ",nameport,proto_rp ? proto_rp : "NULL"));
	}
	else
	{
		name_rp = get_real_address (nameport);
		TRACE(("getservbyname(%s,%s) -> ",name_rp,proto_rp ? proto_rp : "NULL"));
	}

	args[0] = (uae_u32) sb;
	args[1] = 2;
	args[2] = nameport;
	args[3] = proto;
	args[4] = type;
	args[5] = (uae_u32) &buf[0];

	for (i = 0; i < MAX_GET_THREADS; i++)
	{
		if (threadGetargs[i] ==  (void *) -1)
		{
			threadGetargs[i] = 0;
		}
		if (hGetThreads[i] && !threadGetargs[i]) break;
	}

	if (i >= MAX_GET_THREADS)
	{
		for (i = 0; i < MAX_GET_THREADS; i++)
		{
			if (!hGetThreads[i])
			{
				if ((hGetEvents[i] = CreateEvent(NULL,FALSE,FALSE,NULL)) == NULL || (hGetThreads[i] = (void *)THREAD(thread_get,i)) == NULL)
				{
					hGetThreads[i] = 0;
					write_log ("BSDSOCK: ERROR - Thread/Event creation failed - error code: %d\n",GetLastError());
					seterrno(sb,12);	// ENOMEM
					sb->resultval = -1;
					return;
				}

				break;
			}
		}
	}

	if (i >= MAX_GET_THREADS) write_log ("BSDSOCK: ERROR - Too many getprotobyname()s\n");
	else
	{
		bsdsetpriority (hGetThreads[i]);
		threadGetargs[i] = (uae_u32 *)&args[0];
		SetEvent(hGetEvents[i]);
	}

	sb->eintr = 0;

	while ( threadGetargs[i] != 0 && sb->eintr == 0)
	{
		WAITSIGNAL;
		if (sb->eintr == 1)
			threadGetargs[i] =  (void *) -1;
	}

	CANCELSIGNAL;

	if (!sb->sb_errno)
	{
		s = (SERVENT *)buf;

		// compute total size of servent
		size = 20;
		if (s->s_name != NULL) size += strlen(s->s_name)+1;
		if (s->s_proto != NULL) size += strlen(s->s_proto)+1;

		if (s->s_aliases != NULL)
			while (s->s_aliases[numaliases]) size += strlen(s->s_aliases[numaliases++])+5;

		if (sb->servent)
		{
		uae_FreeMem( context, sb->servent, sb->serventsize );
		}

		sb->servent = uae_AllocMem( context, size, 0 );
		if (!sb->servent)
		{
			write_log ("BSDSOCK: WARNING - getservby%s() ran out of Amiga memory (couldn't allocate %ld bytes)\n",type ? "port" : "name",size);
			seterrno(sb,12); // ENOMEM
			return;
		}

		sb->serventsize = size;

		aptr = sb->servent+20+numaliases*4;

		// transfer servent to Amiga memory
		put_long (sb->servent+4,sb->servent+16);
		put_long (sb->servent+8,(unsigned short)htons(s->s_port));

		for (i = 0; i < numaliases; i++) put_long (sb->servent+16+i*4,addstr(&aptr,s->s_aliases[i]));
		put_long (sb->servent+16+numaliases*4,0);
		put_long (sb->servent,aptr);
		addstr(&aptr,s->s_name);
		put_long (sb->servent+12,aptr);
		addstr(&aptr,s->s_proto);

		TRACE(("OK (%s, %d)\n",s->s_name,(unsigned short)htons(s->s_port)));
		seterrno(sb,0);
	}
	else
	{
		TRACE(("failed (%d)\n",sb->sb_errno));
	}
}

uae_u32 host_gethostname(uae_u32 name, uae_u32 namelen)
{
	return gethostname(get_real_address (name),namelen);
}

#endif
#endif
