 /*
  * UAE - The Un*x Amiga Emulator
  *
  * bsdsocket.library emulation machine-independent part
  *
  * Copyright 1997, 1998 Mathias Ortmann
  *
  * Library initialization code (c) Tauno Taipaleenmaki
  */


#include "sysconfig.h"
#include "sysdeps.h"

#include <assert.h>
#include <stddef.h>

#include "options.h"
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
#include "autoconf.h"

#include "traps.h"

#include "threaddep/thread.h"
#include "bsdsocket_amigaos41.h"

#ifdef BSDSOCKET

#ifdef __AMIGAOS4__
#include <proto/exec.h>
#include <proto/dos.h>
#undef __USE_INLINE__
#include <proto/bsdsocket.h>

#include "od-amiga/win32_handle_emu.h"
#include "od-amiga/win32_thread_emu.h"
#include "od-amiga/thread_socket.h"

static uae_u32 SockLibBase;

#define SOCKPOOLSIZE 128
#define UNIQUE_ID	(-1)


extern APTR amiga_thread_safe_mx;
extern APTR sigqueue_mx;

long curruniqid = 65536;

#define SOCKDEBUG 0

#if SOCKDEBUG
#define DPrintf(fmd,...) Printf(fmd, ##__VA_ARGS__)
#else
#define DPrintf(fmd,...)
#endif

static void copyHostentToGuest (TrapContext *context, const struct hostent *hostent, SB );
static void copyProtoentToGuest (TrapContext *context, const struct protoent *p, SB );


void msg_kill_thread_fn( TrapContext *context );
void msg_socket_fn( TrapContext *context );
void msg_bind_fn( TrapContext *context );
void msg_listen_fn( TrapContext *context );
void msg_accept_fn( TrapContext *context );
void msg_connect_fn( TrapContext *context );
void msg_sendto_fn( TrapContext *context );
void msg_send_fn( TrapContext *context );
void msg_recvfrom_fn( TrapContext *context );
void msg_recv_fn( TrapContext *context );
void msg_shutdown_fn( TrapContext *context );
void msg_setsockopt_fn( TrapContext *context );
void msg_getsockopt_fn( TrapContext *context );
void msg_getsockname_fn( TrapContext *context );
void msg_getpeername_fn( TrapContext *context );
void msg_IoctlSocket_fn( TrapContext *context );
void msg_CloseSocket_fn( TrapContext *context );
void msg_WaitSelect_fn( TrapContext *context );
void msg_ObtainSocket_fn( TrapContext *context );
void msg_ReleaseSocket_fn( TrapContext *context );
void msg_Errno_fn( TrapContext *context );
void msg_SetErrnoPtr_fn( TrapContext *context );
void msg_Inet_NtoA_fn( TrapContext *context );
void msg_Inet_addr_fn( TrapContext *context );
void msg_Inet_LnaOf_fn( TrapContext *context );
void msg_Inet_NetOf_fn( TrapContext *context );
void msg_Inet_MakeAddr_fn( TrapContext *context );
void msg_inet_network_fn( TrapContext *context );
void msg_gethostbyname_fn( TrapContext *context );
void msg_gethostbyaddr_fn( TrapContext *context );
void msg_getservbyname_fn( TrapContext *context );
void msg_getservbyport_fn( TrapContext *context );
void msg_getprotobyname_fn( TrapContext *context );
void msg_getprotobynumber_fn( TrapContext *context );
void msg_Dup2Socket_fn( TrapContext *context );
void msg_gethostname_fn( TrapContext *context );
void msg_SocketBaseTagList_fn(TrapContext *context);

/* Memory-related helper functions */
STATIC_INLINE void memcpyha (uae_u32 dst, const char *src, int size)
{
	while (size--)
	put_byte (dst++, *src++);
}

uae_u32 strcpyha (uae_u32 dst, const char *src)
{
	uae_u32 res = dst;

	do {
	put_byte (dst++, *src);
	} while (*src++);

	return res;
}

uae_u32 strncpyha (uae_u32 dst, const char *src, int size)
{
	uae_u32 res = dst;
	while (size--) {
	put_byte (dst++, *src);
	if (!*src++)
		return res;
	}
	return res;
}

uae_u32 addstr (uae_u32 * dst, const char *src)
{
	uae_u32 res = *dst;
	int len;

	len = strlen (src) + 1;

	strcpyha (*dst, src);
	(*dst) += len;

	return res;
}

uae_u32 addmem (uae_u32 * dst, const char *src, int len)
{
	uae_u32 res = *dst;

	if (!src)
	return 0;

	memcpyha (*dst, src, len);
	(*dst) += len;

	return res;
}


/* Get current task */
static uae_u32 gettask (TrapContext *context)
{
	DPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	uae_u32 currtask, a1 = m68k_areg (&context->regs, 1);

	m68k_areg (&context->regs, 1) = 0;
	currtask = CallLib (context, get_long (4), -0x126);	/* FindTask */

	m68k_areg (&context->regs, 1) = a1;

	TRACE (("real name: [%s] ", get_real_address (get_long (currtask + 10))));
	return currtask;
}

/* errno/herrno setting */

#endif

void bsdsocklib_seterrno (SB, int sb_errno)
{
	DPrintf("%s:%s:%ld --- Legacy !!\n",__FILE__,__FUNCTION__,__LINE__);

	sb->sb_errno = sb_errno;

	if (sb->sb_errno >= 1001 && sb->sb_errno <= 1005)
	bsdsocklib_setherrno (sb, sb->sb_errno - 1000);

	if (sb->errnoptr)
	{
		switch (sb->errnosize)
		{
			 case 1:
				put_byte (sb->errnoptr, sb_errno);
				break;

			 case 2:
				put_word (sb->errnoptr, sb_errno);
				break;

			 case 4:
				put_long (sb->errnoptr, sb_errno);
		}
	}
}

void bsdsocklib_setherrno (SB, int sb_herrno)
{
	DPrintf("%s:%s:%ld --- Legacy !!\n",__FILE__,__FUNCTION__,__LINE__);

	sb->sb_herrno = sb_herrno;

	if (sb->herrnoptr)
	{
		switch (sb->herrnosize)
		{
			 case 1:
				put_byte (sb->herrnoptr, sb_herrno);
				break;

			 case 2:
				put_word (sb->herrnoptr, sb_herrno);
				break;

			 case 4:
				put_long (sb->herrnoptr, sb_herrno);
		}
	}
}

BOOL checksd (SB, int sd)
{
	DPrintf("%s:%s:%ld --- Legacy !!\n",__FILE__,__FUNCTION__,__LINE__);

	 int iCounter;
	 SOCKET s;

	 s = getsock (sb, sd);
	 if (s != INVALID_SOCKET)
	{
		for (iCounter  = 1; iCounter <= sb->dtablesize; iCounter++) {
			if (iCounter != sd)
			{
				if (getsock (sb, iCounter) == s)
				{
					releasesock (sb, sd);
					return 1;
				}
			}
		}
	}
	TRACE (("checksd FALSE s 0x%x sd %d\n", s, sd));
	return 0;
}

void setsd (SB, int sd, int s)
{
	DPrintf("%s:%s:%ld --- Legacy !!\n",__FILE__,__FUNCTION__,__LINE__);
	sb->dtable[sd - 1] = s;
}

/* Socket descriptor/opaque socket handle management */
int getsd (SB, int s)
{
	DPrintf("%s:%s:%ld --- Legacy !! \n",__FILE__,__FUNCTION__,__LINE__);

	int i;
	int *dt = sb->dtable;

	/* return socket descriptor if already exists */
	for (i = sb->dtablesize; i--;)
	if (dt[i] == s)
		return i + 1;

	/* create new table entry */
	for (i = 0; i < sb->dtablesize; i++)
	if (dt[i] == -1) {
		dt[i] = s;
		sb->ftable[i] = SF_BLOCKING;
		return i + 1;
	}
	/* descriptor table full. */
	bsdsocklib_seterrno (sb, 24);		/* EMFILE */

	return -1;
}

int getsock (SB, int sd)
{
	DPrintf("%s:%s:%ld -- legacy !!\n",__FILE__,__FUNCTION__,__LINE__);

	if ((unsigned int) (sd - 1) >= (unsigned int) sb->dtablesize) {
	TRACE (("Invalid Socket Descriptor (%d, %d)\n", sd - 1, sb->dtablesize));
	bsdsocklib_seterrno (sb, 38);	/* ENOTSOCK */

	return -1;
	}
	return sb->dtable[sd - 1];
}

void releasesock (SB, int sd)
{
	DPrintf("%s:%s:%ld -- legacy !!\n",__FILE__,__FUNCTION__,__LINE__);

	if ((unsigned int) (sd - 1) < (unsigned int) sb->dtablesize)
	sb->dtable[sd - 1] = -1;
}

/* Signal queue */
/* @@@ TODO: ensure proper interlocking */

struct socketbase *sbsigqueue;

BOOL open_thread_per_library(struct MsgPort **retPort, int *retIndex )
{
	int index;

	DPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	index = find_new_thread_id();
	if (index>-1)
	{
		struct handle_thread_s *t;

		hThreads[index].ptr = (struct handle_s *)  new_thread( socket_proxy_thread, index );
		t = (struct handle_thread_s *) hThreads[index].ptr;

		if (t)
		{
			while (( t -> t.ProxyPort == NULL ))
			{
				Delay(1);
			}

			*retPort = t -> t.ProxyPort;
			*retIndex = index;

			hThreads[index].lock = 0;	// unlock..
			return TRUE;
		}

		hThreads[index].lock = 0;	// unlock
	}

	return FALSE;
}


/* Allocate and initialize per-task state structure */
static struct socketbase *alloc_socketbase (TrapContext *context)
{
	struct socketbase *sb;
	int i;

	DPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	if ((sb = calloc (sizeof (struct socketbase), 1)) != NULL)
	{
		sb->ownertask = gettask (context);

		m68k_dreg (&context->regs, 0) = -1;
		sb->signal = CallLib (context, get_long (4), -0x14A);

		if (sb->signal == -1)
		{
			write_log ("bsdsocket: ERROR: Couldn't allocate signal for task 0x%lx.\n", sb->ownertask);
			free (sb);
			return NULL;
		}

		m68k_dreg (&context->regs, 0) = SCRATCHBUFSIZE;
		m68k_dreg (&context->regs, 1) = 0;

		sb->dtablesize = DEFAULT_DTABLE_SIZE;
		/* @@@ check malloc() result */

		// Descriptor table (holds sockets)
		sb->dtable = malloc (sb->dtablesize * sizeof (*sb->dtable));

		// socket flags table 
		sb->ftable = malloc (sb->dtablesize * sizeof (*sb->ftable));

		open_thread_per_library( &(sb -> ProxyPort), &(sb -> thread_id) );

		return sb;
	}
	return NULL;
}

STATIC_INLINE struct socketbase *get_socketbase (TrapContext *context)
{
	return get_pointer (m68k_areg (&context->regs, 6) + offsetof (struct UAEBSDBase, sb));
}

extern APTR amiga_thread_safe_mx;

void locksigqueue()
{
	MutexObtain(sigqueue_mx);
}

void unlocksigqueue()
{
	MutexRelease(sigqueue_mx);
}

uae_u32 guest_AllocMem( TrapContext *context, uae_u32 size, uae_u32 type )
{
	m68k_dreg (&context->regs, 0) = size;
	m68k_dreg (&context->regs, 1) = type;
	return CallLib ( context, get_long (4), -0xC6);
}

void guest_FreeMem( TrapContext *context, ULONG ptr, ULONG size )
{
	m68k_areg (&context->regs, 1) = ptr;
	m68k_dreg (&context->regs, 0) = size;
	CallLib (context, get_long (4), -0xD2);	/* FreeMem */
}

static void free_socketbase (TrapContext *context)
{
	struct socketbase *sb, *nsb;

	DPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	if ((sb = get_socketbase (context)) != NULL)
	{
		m68k_dreg (&context->regs, 0) = sb->signal;
		CallLib (context, get_long (4), -0x150);		/* FreeSignal */

		if (sb->hostent) 
			guest_FreeMem( context, sb->hostent, sb->hostentsize );

		if (sb->protoent) 
			guest_FreeMem( context, sb->protoent, sb->protoentsize );

		if (sb->servent) 
			guest_FreeMem( context, sb->servent, sb->serventsize );

		socketbase_cleanup (sb);
		locksigqueue ();

		if (sb == socketbases)
			socketbases = sb->next;
		else
		{
			for (nsb = socketbases; nsb; nsb = nsb->next)
			{
				if (sb == nsb->next)
				{
					nsb->next = sb->next;
					break;
				}
			}
		}

		if (sb == sbsigqueue)
			sbsigqueue = sb->next;
		else
		{
			for (nsb = sbsigqueue; nsb; nsb = nsb->next)
			{
				if (sb == nsb->next)
				{
					nsb->next = sb->next;
					break;
				}
			}
		}

		unlocksigqueue ();

		free (sb);
	}
}

static uae_u32 REGPARAM2 bsdsocklib_Expunge (TrapContext *context)
{
//	DPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	TRACE (("Expunge() -> [ignored]\n"));
	return 0;
}

static uae_u32 functable, datatable, inittable;

extern struct Task *main_task;

static uae_u32 REGPARAM2 bsdsocklib_Open (TrapContext *context)
{
	struct Task *self;

	uae_u32 result = 0;
	int opencount;
	unsigned int i;
	struct socketbase *sb;
	uae_u32 p[sizeof (void*) / 4];

	TRACE (("OpenLibrary() -> "));

	if ((sb = alloc_socketbase (context)) != NULL)
	{
		DPrintf("%s:%s:%ld - SockLibBase: %p\n",__FILE__,__FUNCTION__,__LINE__, SockLibBase);

		put_word (SockLibBase + 32, opencount = get_word (SockLibBase + 32) + 1);

		m68k_areg (&context->regs, 0) = functable;
		m68k_areg (&context->regs, 1) = datatable;
		m68k_areg (&context->regs, 2) = 0;
		m68k_dreg (&context->regs, 0) = sizeof (struct UAEBSDBase);
		m68k_dreg (&context->regs, 1) = 0;

		result = CallLib (context, get_long (4), -0x54); // MakeLibrary

		// store socketbase in bsd base.
		put_pointer (result + offsetof (struct UAEBSDBase, sb), sb);
	}
	else
	{
		TRACE (("failed (out of memory)\n"));
	}

	return result;
}

void msg_kill_thread_fn( TrapContext *context )
{
	DPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	struct handle_thread_s *thread = GetCurrentThread();
	thread -> t.running = FALSE;
}

void kill_thread( TrapContext *context )
{
	struct socketbase *sb = get_socketbase (context);
	send_thread_msg( sb, context, msg_kill_thread_fn );
}

static uae_u32 REGPARAM2 bsdsocklib_Close (TrapContext *context)
{
	int opencount;

	DPrintf("%p:%s:%s:%ld\n",FindTask(NULL),__FILE__,__FUNCTION__,__LINE__);

	kill_thread (context);

	MutexObtain(amiga_thread_safe_mx);

	uae_u32 base = m68k_areg (&context->regs, 6);
	uae_u32 negsize = get_word (base + 16);

	free_socketbase (context);

	put_word (SockLibBase + 32, opencount = get_word (SockLibBase + 32) - 1);

	m68k_areg (&context->regs, 1) = base - negsize;
	m68k_dreg (&context->regs, 0) = negsize + get_word (base + 18);
	CallLib (context, get_long (4), -0xD2);	/* FreeMem */

	TRACE (("CloseLibrary() -> [%d]\n", opencount)); // <--- THIS CRASHES???

	MutexRelease(amiga_thread_safe_mx);

	return 0;
}

#define DREG(n) regs[n]
#define AREG(n) (APTR *) get_real_address (regs[n+8])

#define guest_AREG(n) regs[n+8]

void msg_socket_fn( TrapContext *context )
{
	DPrintf("%s:%ld\n",__FUNCTION__,__LINE__);

	int socket_id, guest_socket_id;
	ULONG *regs = (ULONG *) context -> regs.regs;
	struct handle_thread_s *thread = GetCurrentThread() ;
	struct socketbase *sb = get_socketbase (context);

	guest_socket_id = new_guest_socket();

	if (guest_socket_id>-1)
	{
		DPrintf("tmp socket id: %ld\n",guest_socket_id);

		DPrintf("D0: %08lx, D1: %08lx, D2: %08lx\n",DREG(0), DREG(1), DREG(2) );

		socket_id = thread_socket ( DREG(0), DREG(1), DREG(2) );

		if (socket_id>-1)
		{
			DPrintf("host socket id: %08lx\n",socket_id);

			sockets[guest_socket_id].host_socket = socket_id;
			sockets[guest_socket_id].ProxyPort = ((struct handle_thread_s *) hThreads[ sb -> thread_id].ptr) -> t.ProxyPort;
			DREG(0) = guest_socket_id;
			return;
		}
		else
		{
			DPrintf("Socket error\n");

			switch( thread_Errno() )
			{
				case EPROTONOSUPPORT:
					DPrintf("EPROTONOSUPPORT\n");
					break;

				case EMFILE:
					DPrintf("EMFILE\n");
					break;

				case EACCES:
					DPrintf("EACCES\n");
					break;

				case ENOBUFS:
					DPrintf("ENOBUFS\n");
					break;
			}
		}
	}
	else
	{
		DPrintf("Failed to find new guest socket\n");
	}

	DREG(0) = -1;
}

/* socket(domain, type, protocol)(d0/d1/d2) */
static uae_u32 REGPARAM2 bsdsocklib_socket (TrapContext *context)
{
	struct socketbase *sb;
	sb = get_socketbase (context);
	send_thread_msg( sb, context, msg_socket_fn );
	return context -> regs.regs[0];
}

void msg_bind_fn( TrapContext *context )
{
	DPrintf("%s:%ld\n",__FUNCTION__,__LINE__);

	ULONG *regs = (ULONG *) context -> regs.regs;
	struct handle_thread_s *thread = GetCurrentThread() ;
	struct sockaddr *sockaddr = (struct sockaddr *) AREG(0);
	DREG(0) =  thread_bind ( DREG(0), sockaddr , DREG(1) );
}

/* bind(s, name, namelen)(d0/a0/d1) */
static uae_u32 REGPARAM2 bsdsocklib_bind (TrapContext *context)
{
	struct socketbase *sb = get_socketbase (context);
	send_thread_msg( sb, context, msg_bind_fn );
	return context -> regs.regs[0];
}

void msg_listen_fn( TrapContext *context )
{
	DPrintf("%s:%ld\n",__FUNCTION__,__LINE__);

	struct socketbase *sb = get_socketbase (context);
	ULONG *regs = (ULONG *) context -> regs.regs;
	struct handle_thread_s *thread = GetCurrentThread() ;
	DREG(0) = thread_listen ( DREG(0), DREG(1) );
}

/* listen(s, backlog)(d0/d1) */
static uae_u32 REGPARAM2 bsdsocklib_listen (TrapContext *context)
{
	struct socketbase *sb = get_socketbase (context);
	send_thread_msg( sb, context, msg_listen_fn );
	return context -> regs.regs[0];
}

void msg_accept_fn( TrapContext *context )
{
	DPrintf("%s:%ld\n",__FUNCTION__,__LINE__);

	struct socketbase *sb = get_socketbase (context);
	ULONG *regs = (ULONG *) context -> regs.regs;
	struct handle_thread_s *thread = GetCurrentThread() ;

	APTR paddr = (APTR) guest_AREG(0);
	socklen_t *socklen = (socklen_t *) guest_AREG(1);

	if (paddr) paddr = (APTR) get_real_address( (uae_u32) paddr);
	if (socklen) socklen = (socklen_t *) get_real_address( (uae_u32) socklen );

	DREG(0) = thread_accept ( DREG(0), paddr, socklen );
}

/* accept(s, addr, addrlen)(d0/a0/a1) */
static uae_u32 REGPARAM2 bsdsocklib_accept (TrapContext *context)
{
	struct socketbase *sb = get_socketbase (context);
	send_thread_msg( sb, context, msg_accept_fn );
	return sb->resultval;
}

void msg_connect_fn( TrapContext *context )
{
	DPrintf("%s:%ld\n",__FUNCTION__,__LINE__);

	struct socketbase *sb = get_socketbase (context);
	ULONG *regs = (ULONG *) context -> regs.regs;
	struct handle_thread_s *thread = GetCurrentThread();

	struct sockaddr *sockaddr = (struct sockaddr *) AREG(0);

	DREG(0) = thread_connect ( DREG(0), sockaddr, DREG(1) );
}

/* connect(s, name, namelen)(d0/a0/d1) */
static uae_u32 REGPARAM2 bsdsocklib_connect (TrapContext *context)
{
	struct socketbase *sb = get_socketbase (context);
	send_thread_msg( sb, context, msg_connect_fn );
	return m68k_dreg (&context->regs, 0);
}

void msg_sendto_fn( TrapContext *context )
{
	DPrintf("%s:%ld\n",__FUNCTION__,__LINE__);

	struct socketbase *sb = get_socketbase (context);
	ULONG *regs = (ULONG *) context -> regs.regs;
	struct handle_thread_s *thread = GetCurrentThread();

	struct sockaddr *sockaddr_to = (struct sockaddr *) guest_AREG(1);
	if (sockaddr_to) sockaddr_to = (struct sockaddr *) get_real_address( (uae_u32) sockaddr_to);

	DREG(0) = thread_sendto ( DREG(0), AREG(0), DREG(1), DREG(2), sockaddr_to, DREG(3) );
}

/* sendto(s, msg, len, flags, to, tolen)(d0/a0/d1/d2/a1/d3) */
static uae_u32 REGPARAM2 bsdsocklib_sendto (TrapContext *context)
{
	struct socketbase *sb = get_socketbase (context);
	send_thread_msg( sb, context, msg_sendto_fn );
	return m68k_dreg (&context->regs, 0);
}

void msg_send_fn( TrapContext *context )
{
	DPrintf("%s:%ld\n",__FUNCTION__,__LINE__);

	struct socketbase *sb = get_socketbase (context);
	ULONG *regs = (ULONG *) context -> regs.regs;
	struct handle_thread_s *thread = GetCurrentThread();

	DREG(0) = thread_send ( DREG(0), AREG(0), DREG(1), DREG(2) );
}

/* send(s, msg, len, flags)(d0/a0/d1/d2) */
static uae_u32 REGPARAM2 bsdsocklib_send (TrapContext *context)
{
	struct socketbase *sb = get_socketbase (context);
	send_thread_msg( sb, context, msg_send_fn );
	return m68k_dreg (&context->regs, 0);
}

#define ptr_host_arg(type,addr) (type) (addr ? get_real_address( (uaecptr) addr ) : NULL)

void msg_recvfrom_fn( TrapContext *context )
{
	DPrintf("%s:%ld\n",__FUNCTION__,__LINE__);

	struct socketbase *sb = get_socketbase (context);
	ULONG *regs = (ULONG *) context -> regs.regs;
	struct handle_thread_s *thread = GetCurrentThread();

	struct sockaddr *guest_sockaddr_from = (struct sockaddr *) guest_AREG(1);
	socklen_t *guest_fromlen = (socklen_t *) guest_AREG(2);
	
	DPrintf("%p:%s:%s:%ld\n",FindTask(NULL),__FILE__,__FUNCTION__,__LINE__);

	DPrintf("D0: %08lx, A0: %p, D1: len=%ld, D2: %08lx, A1: %p, A2: %p\n",
			DREG(0),
			guest_AREG(0),
			DREG(1),
			DREG(2),
			guest_AREG(1),
			guest_AREG(2));

	m68k_dreg (&context->regs, 0) = thread_recvfrom ( DREG(0), AREG(0), DREG(1), DREG(2),
			ptr_host_arg( struct sockaddr *, guest_sockaddr_from ),
			ptr_host_arg( socklen_t *, guest_fromlen ));

	DPrintf("recvfrom() = %ld, errno %ld\n", m68k_dreg (&context->regs, 0), thread_Errno() );

}

/* recvfrom(s, buf, len, flags, from, fromlen)(d0/a0/d1/d2/a1/a2) */
static uae_u32 REGPARAM2 bsdsocklib_recvfrom (TrapContext *context)
{
	struct socketbase *sb = get_socketbase (context);
	send_thread_msg( sb, context, msg_recvfrom_fn );
	return m68k_dreg (&context->regs, 0);
}

void msg_recv_fn( TrapContext *context )
{
	DPrintf("%s:%ld\n",__FUNCTION__,__LINE__);

	struct socketbase *sb = get_socketbase (context);
	ULONG *regs = (ULONG *) context -> regs.regs;
	struct handle_thread_s *thread = GetCurrentThread();
	m68k_dreg (&context->regs, 0) = thread_recv ( DREG(0), AREG(0), DREG(1), DREG(2));
}

/* recv(s, buf, len, flags)(d0/a0/d1/d2) */
static uae_u32 REGPARAM2 bsdsocklib_recv (TrapContext *context)
{
	struct socketbase *sb = get_socketbase (context);
	send_thread_msg( sb, context, msg_recv_fn );
	return m68k_dreg (&context->regs, 0);
}

void msg_shutdown_fn( TrapContext *context )
{
	DPrintf("%s:%ld\n",__FUNCTION__,__LINE__);

	struct socketbase *sb = get_socketbase (context);
	ULONG *regs = (ULONG *) context -> regs.regs;
	struct handle_thread_s *thread = GetCurrentThread();
	DREG(0) =  thread_shutdown ( DREG(0), DREG(1) );
}

/* shutdown(s, how)(d0/d1) */
static uae_u32 REGPARAM2 bsdsocklib_shutdown (TrapContext *context)
{
	ULONG *regs = (ULONG *) context -> regs.regs;
	struct socketbase *sb = get_socketbase (context);

	DPrintf("%s:%ld\n",__FUNCTION__,__LINE__);
	send_thread_msg( sb, context, msg_shutdown_fn );

	return m68k_dreg (&context->regs, 0);
}

void msg_setsockopt_fn( TrapContext *context )
{
	DPrintf("%s:%ld\n",__FUNCTION__,__LINE__);

	ULONG *regs = (ULONG *) context -> regs.regs;
	struct handle_thread_s *thread = GetCurrentThread();
	DREG(0) = thread_setsockopt ( DREG(0), DREG(1), DREG(2), AREG(0), DREG(3) );
}

/* setsockopt(s, level, optname, optval, optlen)(d0/d1/d2/a0/d3) */
static uae_u32 REGPARAM2 bsdsocklib_setsockopt (TrapContext *context)
{
	struct socketbase *sb = get_socketbase (context);
	send_thread_msg( sb, context, msg_setsockopt_fn );
	return m68k_dreg (&context->regs, 0);
}

void msg_getsockopt_fn( TrapContext *context )
{
	DPrintf("%s:%ld\n",__FUNCTION__,__LINE__);

	ULONG *regs = (ULONG *) context -> regs.regs;
	struct handle_thread_s *thread = GetCurrentThread();

	LONG sock = DREG(0);
	LONG level = DREG(1);
	LONG optname = DREG(2);
	APTR optval = (APTR) guest_AREG(0);
	socklen_t * optlen = (socklen_t *) guest_AREG(1);

	if (optval) optval = (APTR) get_real_address( (uae_u32) optval);
	if (optlen) optlen = (socklen_t *) get_real_address( (uae_u32) optval);

	DREG(0) = thread_getsockopt ( sock, level, optname, optval, optlen );
}

/* getsockopt(s, level, optname, optval, optlen)(d0/d1/d2/a0/a1) */
static uae_u32 REGPARAM2 bsdsocklib_getsockopt (TrapContext *context)
{
	struct socketbase *sb = get_socketbase (context);
	send_thread_msg( sb, context, msg_getsockopt_fn );
	return m68k_dreg (&context->regs, 0);
}

void msg_getsockname_fn( TrapContext *context )
{
	DPrintf("%s:%ld\n",__FUNCTION__,__LINE__);
	ULONG *regs = (ULONG *) context -> regs.regs;
	struct handle_thread_s *thread = GetCurrentThread();
	DREG(0) = thread_getsockname ( DREG(0), (struct sockaddr *) AREG(0), (socklen_t *) AREG(1) );
}

/* getsockname(s, hostname, namelen)(d0/a0/a1) */
static uae_u32 REGPARAM2 bsdsocklib_getsockname (TrapContext *context)
{
	struct socketbase *sb = get_socketbase (context);
	send_thread_msg( sb, context, msg_getsockname_fn );
	return m68k_dreg (&context->regs, 0);
}

void msg_getpeername_fn( TrapContext *context )
{
	DPrintf("%s:%ld\n",__FUNCTION__,__LINE__);
	ULONG *regs = (ULONG *) context -> regs.regs;
	struct handle_thread_s *thread = GetCurrentThread();
	DREG(0) = thread_getsockname ( DREG(0), (struct sockaddr *) AREG(0), (socklen_t *) AREG(1) );
}

/* getpeername(s, hostname, namelen)(d0/a0/a1) */
static uae_u32 REGPARAM2 bsdsocklib_getpeername (TrapContext *context)
{
	struct socketbase *sb = get_socketbase (context);
	send_thread_msg( sb, context, msg_getpeername_fn );
	return m68k_dreg (&context->regs, 0);
}

void msg_IoctlSocket_fn( TrapContext *context )
{
	ULONG *regs = (ULONG *) context -> regs.regs;
	struct handle_thread_s *thread = GetCurrentThread();

	DPrintf("%p:%s:%s:%ld\n",FindTask(NULL),__FILE__,__FUNCTION__,__LINE__);

	DPrintf("D0: fd = %08lx , D1: flags = %08lx, A0: FIONBIO = %ld\n", DREG(0), DREG(1), guest_AREG(0) );
	DREG(0) = thread_IoctlSocket ( DREG(0), DREG(1), AREG(0) );

	DPrintf("Result: %d\n", DREG(0) );
}

/* *------ generic system calls related to sockets */
/* IoctlSocket(d, request, argp)(d0/d1/a0) */
static uae_u32 REGPARAM2 bsdsocklib_IoctlSocket (TrapContext *context)
{
	struct socketbase *sb = get_socketbase (context);
	send_thread_msg( sb, context, msg_IoctlSocket_fn );
	return m68k_dreg (&context->regs, 0);
}

void msg_CloseSocket_fn( TrapContext *context )
{
	DPrintf("%s:%ld\n",__FUNCTION__,__LINE__);

	ULONG *regs = (ULONG *) context -> regs.regs;
	struct handle_thread_s *thread = GetCurrentThread();
	int guest_socket_id = DREG(0);
	DREG(0) = thread_CloseSocket( sockets[guest_socket_id]. host_socket  );
	free_guest_socket( guest_socket_id );
}

/* *------ AmiTCP/IP specific stuff */
/* CloseSocket(d)(d0) */
static uae_u32 REGPARAM2 bsdsocklib_CloseSocket (TrapContext *context)
{
	struct socketbase *sb = get_socketbase (context);
	send_thread_msg( sb, context, msg_CloseSocket_fn );
	return m68k_dreg (&context->regs, 0);
}

void msg_WaitSelect_fn( TrapContext *context )
{
	DPrintf("%s:%ld\n",__FUNCTION__,__LINE__);
	ULONG ret;
	ULONG *regs = (ULONG *) context -> regs.regs;
	struct handle_thread_s *thread = GetCurrentThread();

	LONG nfds = DREG(0);
	APTR guest_read_fds = (fd_set *) guest_AREG(0);			// optional args
	APTR guest_write_fds = (fd_set *) guest_AREG(1);			// optional args
	APTR guest_except_fds = (fd_set *) guest_AREG(2);		// optional args
	struct timeval * timeout = (struct timeval *) guest_AREG(3);	// optional args
	uae_u32 guest_signal_mask = regs[5]; // D5: guest signal bits

	int max_host_fd = -1;
	fd_set host_read_fds, host_write_fds, host_except_fds;

	// convert only address if guest address is not NULL...
	if (timeout) timeout = (struct timeval *) get_real_address( (uae_u32) timeout );

	FD_ZERO(&host_read_fds);
	FD_ZERO(&host_write_fds);
	FD_ZERO(&host_except_fds);

	for (int guest_fd = 0; guest_fd < nfds; guest_fd++)
	{
		if (guest_fd == -1) continue;

		int host_fd = to_host_sock(guest_fd);

		if (host_fd == -1) continue;
		
		if (guest_read_fds && FD_ISSET( guest_fd, (fd_set *) get_real_address( (uae_u32) guest_read_fds)))
			FD_SET(host_fd, &host_read_fds);

		if (guest_write_fds && FD_ISSET(guest_fd, (fd_set *) get_real_address( (uae_u32) guest_write_fds)))
			FD_SET(host_fd, &host_write_fds);

		if (guest_except_fds && FD_ISSET(guest_fd, (fd_set *) get_real_address( (uae_u32) guest_except_fds)))
			FD_SET(host_fd, &host_except_fds);

		if (host_fd > max_host_fd)
			max_host_fd = host_fd;
	}

#if 0
	if (guest_signal_mask) // wait for guest signals.
	{
		m68k_dreg (&context->regs, 0) = guest_signal_mask;
		ULONG sigs = CallLib (context, get_long (4), -0x13e);	// Wait()
	}
#else
#warning missing signal support in WaitSelect....
#endif

	ret = thread_WaitSelect ( max_host_fd +1, &host_read_fds, &host_write_fds, &host_except_fds, timeout, NULL );

	if (ret > 0)
	{
		// Sockets are ready â update guest fd_sets
		if (guest_read_fds) FD_ZERO( (fd_set *) get_real_address( (uae_u32) guest_read_fds));
		if (guest_write_fds) FD_ZERO( (fd_set *) get_real_address( (uae_u32) guest_write_fds));
		if (guest_except_fds) FD_ZERO( (fd_set *) get_real_address( (uae_u32) guest_except_fds));

		for (int guest_fd = 0; guest_fd < nfds; guest_fd++)
		{
			int host_fd = to_host_sock(guest_fd);
			if (host_fd == -1) continue;

			if (guest_read_fds && FD_ISSET(host_fd, &host_read_fds))
				FD_SET(guest_fd, (fd_set *) get_real_address( (uae_u32) guest_read_fds));

			if (guest_write_fds && FD_ISSET(host_fd, &host_write_fds))
				FD_SET(guest_fd, (fd_set *) get_real_address( (uae_u32) guest_write_fds));

			if (guest_except_fds && FD_ISSET(host_fd, &host_except_fds))
				FD_SET(guest_fd, (fd_set *) get_real_address( (uae_u32) guest_except_fds));
		}
	}

	DREG(0) = ret;
}

/* WaitSelect(nfds, readfds, writefds, execptfds, timeout, maskp)(d0/a0/a1/a2/a3/d1) */
static uae_u32 REGPARAM2 bsdsocklib_WaitSelect (TrapContext *context)
{
	struct socketbase *sb = get_socketbase (context);
	send_thread_msg( sb, context, msg_WaitSelect_fn );
	return m68k_dreg (&context->regs, 0);
}

/* SetSocketSignals(SIGINTR, SIGIO, SIGURG)(d0/d1/d2) */
static uae_u32 REGPARAM2 bsdsocklib_SetSocketSignals (TrapContext *context)
{
	struct socketbase *sb = get_socketbase (context);

	DPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	TRACE (("SetSocketSignals(0x%08lx,0x%08lx,0x%08lx) -> ", m68k_dreg (&context->regs, 0), m68k_dreg (&context->regs, 1), m68k_dreg (&context->regs, 2)));
	sb->eintrsigs = m68k_dreg (&context->regs, 0);
	sb->eventsigs = m68k_dreg (&context->regs, 1);
	return 0;
}

/* SetDTableSize(size)(d0) */
static uae_u32 bsdsocklib_SetDTableSize (SB, int newSize)
{
	int *newdtable;
	int *newftable;
	int i;

	DPrintf("%p:%s:%s:%ld\n",FindTask(NULL),__FILE__,__FUNCTION__,__LINE__);

	if (newSize < sb->dtablesize)
	{
		/* I don't support lowering the size */
		return 0;
	}

	newdtable = (int *)malloc(newSize * sizeof(*sb->dtable));
	newftable = (int *)malloc(newSize * sizeof(*sb->ftable));

	if (newdtable == NULL || newftable == NULL)
	{
		sb->resultval = -1;
		bsdsocklib_seterrno(sb, ENOMEM);
		return -1;
	}

	memcpy(newdtable, sb->dtable, sb->dtablesize * sizeof(*sb->dtable));
	memcpy(newftable, sb->ftable, sb->dtablesize * sizeof(*sb->ftable));

	for (i = sb->dtablesize + 1; i < newSize; i++)
		newdtable[i] = -1;

	sb->dtablesize = newSize;
	free(sb->dtable);
	free(sb->ftable);
	sb->dtable = newdtable;
	sb->ftable = newftable;
	sb->resultval = 0;
	return 0;
}

void msg_ObtainSocket_fn( TrapContext *context )
{
	DPrintf("%s:%ld\n",__FUNCTION__,__LINE__);

	struct socketbase *sb = get_socketbase (context);
	ULONG *regs = (ULONG *) context -> regs.regs;

	struct handle_thread_s *thread = GetCurrentThread();
	int host_socket_id, guest_socket_id =  new_guest_socket();

	if ( guest_socket_id != -1 )
	{
		host_socket_id = thread_ObtainSocket( DREG(0),DREG(1),DREG(2),DREG(3)  );

		if (host_socket_id>-1)
		{
			sockets[guest_socket_id].host_socket = host_socket_id;
			sockets[guest_socket_id].ProxyPort = ((struct handle_thread_s *) hThreads[ sb -> thread_id].ptr) -> t.ProxyPort;
			DREG(0) = guest_socket_id;
			return;
		}
	}

	DREG(0) = -1;
	return;
}

/* ObtainSocket(id, domain, type, protocol)(d0/d1/d2/d3) */
static uae_u32 REGPARAM2 bsdsocklib_ObtainSocket (TrapContext *context)
{
	struct socketbase *sb = get_socketbase (context);
	send_thread_msg( sb, context, msg_ObtainSocket_fn );
	return m68k_dreg (&context->regs, 0);
}

void msg_ReleaseSocket_fn( TrapContext *context )
{
	DPrintf("%s:%ld\n",__FUNCTION__,__LINE__);

	struct socketbase *sb = get_socketbase (context);
	ULONG *regs = (ULONG *) context -> regs.regs;

	struct handle_thread_s *thread = GetCurrentThread();
	int host_socket_id, guest_socket_id =  DREG(0);

	DREG(0) = -1; // default is a error...
	if ( guest_socket_id != -1 )
	{
		host_socket_id = sockets[guest_socket_id].host_socket;

		DREG(0) = thread_ReleaseSocket( host_socket_id,DREG(1) );

		if (DREG(0) != -1)	// remove socket..
		{
			sockets[guest_socket_id].host_socket = 0;
			sockets[guest_socket_id].ProxyPort = NULL;
		}
	}

	return;
}

/* ReleaseSocket(fd, id)(d0/d1) */
static uae_u32 REGPARAM2 bsdsocklib_ReleaseSocket (TrapContext *context)
{
	struct socketbase *sb = get_socketbase (context);
	send_thread_msg( sb, context, msg_ReleaseSocket_fn );
	return m68k_dreg (&context->regs, 0);
}

/* ReleaseCopyOfSocket(fd, id)(d0/d1) */
static uae_u32 REGPARAM2 bsdsocklib_ReleaseCopyOfSocket (TrapContext *context)
{
	write_log ("bsdsocket: UNSUPPORTED: ReleaseCopyOfSocket()\n");
	return 0;
}

void msg_Errno_fn( TrapContext *context )
{
	DPrintf("%s:%ld\n",__FUNCTION__,__LINE__);
	ULONG *regs = (ULONG *) context -> regs.regs;
	struct handle_thread_s *thread = GetCurrentThread() ;
	DREG(0) =  thread_Errno ();
}


/* Errno()() */
static uae_u32 REGPARAM2 bsdsocklib_Errno (TrapContext *context)
{
	struct socketbase *sb = get_socketbase (context);
	send_thread_msg( sb, context, msg_Errno_fn );
	return m68k_dreg (&context->regs, 0);
}

void msg_SetErrnoPtr_fn( TrapContext *context )
{
	DPrintf("%s:%ld\n",__FUNCTION__,__LINE__);
	ULONG *regs = (ULONG *) context -> regs.regs;
	struct handle_thread_s *thread = GetCurrentThread() ;
	thread_SetErrnoPtr ( AREG(0), DREG(0) );
}

/* SetErrnoPtr(errno_p, size)(a0/d0) */
static uae_u32 REGPARAM2 bsdsocklib_SetErrnoPtr (TrapContext *context)
{
	struct socketbase *sb = get_socketbase (context);
	send_thread_msg( sb, context, msg_SetErrnoPtr_fn );
	return m68k_dreg (&context->regs, 0);
}

void msg_Inet_NtoA_fn( TrapContext *context )
{
	ULONG *regs = (ULONG *) context -> regs.regs;
	struct handle_thread_s *thread = GetCurrentThread() ;
	struct in_addr ina;
	char *addr;
	uae_u32 buf;

	DPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	if (( addr  = (char *) thread_Inet_NtoA(DREG(0)) ))
	{
		uae_u32 buf = m68k_areg (&context->regs, 6) + offsetof (struct UAEBSDBase, scratchbuf);
		strncpyha (buf, addr, SCRATCHBUFSIZE);
		context -> regs.regs[0] = (uae_u32) buf; 
	}
	else
	{
		context -> regs.regs[0] = (uae_u32) NULL; 
	}
}

/* *------ inet library calls related to inet address manipulation */
/* Inet_NtoA(in)(d0) */
static uae_u32 REGPARAM2 bsdsocklib_Inet_NtoA (TrapContext *context)
{
	struct socketbase *sb = get_socketbase (context);
	send_thread_msg( sb, context, msg_Inet_NtoA_fn );
	return context -> regs.regs[0];
}

void msg_Inet_addr_fn( TrapContext *context )
{
	DPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	ULONG *regs = (ULONG *) context -> regs.regs;
	struct handle_thread_s *thread = GetCurrentThread() ;
	DREG(0) =  thread_Inet_addr( (char *) AREG(0) );
}

/* inet_addr(cp)(a0) */
static uae_u32 REGPARAM2 bsdsocklib_inet_addr (TrapContext *context)
{
	struct socketbase *sb = get_socketbase (context);
	send_thread_msg( sb, context, msg_Inet_addr_fn );
	return context -> regs.regs[0];
}

void msg_Inet_LnaOf_fn( TrapContext *context )
{
	DPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	ULONG *regs = (ULONG *) context -> regs.regs;
	struct handle_thread_s *thread = GetCurrentThread() ;
	DREG(0) =  thread_Inet_LnaOf( DREG(0) );
}

/* Inet_LnaOf(in)(d0) */
static uae_u32 REGPARAM2 bsdsocklib_Inet_LnaOf (TrapContext *context)
{
	struct socketbase *sb = get_socketbase (context);
	send_thread_msg( sb, context, msg_Inet_LnaOf_fn );
	return context -> regs.regs[0];
}

void msg_Inet_NetOf_fn( TrapContext *context )
{
	DPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	ULONG *regs = (ULONG *) context -> regs.regs;
	struct handle_thread_s *thread = GetCurrentThread() ;
	DREG(0) =  thread_Inet_NetOf( DREG(0) );
}

/* Inet_NetOf(in)(d0) */
static uae_u32 REGPARAM2 bsdsocklib_Inet_NetOf (TrapContext *context)
{
	struct socketbase *sb = get_socketbase (context);
	send_thread_msg( sb, context, msg_Inet_NetOf_fn );
	return context -> regs.regs[0];
}

void msg_Inet_MakeAddr_fn( TrapContext *context )
{
	DPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	ULONG *regs = (ULONG *) context -> regs.regs;
	struct handle_thread_s *thread = GetCurrentThread() ;
	DREG(0) =  thread_inet_MakeAddr ( DREG(0), DREG(1) );
}

/* Inet_MakeAddr(net, host)(d0/d1) */
static uae_u32 REGPARAM2 bsdsocklib_Inet_MakeAddr (TrapContext *context)
{
	struct socketbase *sb = get_socketbase (context);
	send_thread_msg( sb, context, msg_Inet_MakeAddr_fn );
	return context -> regs.regs[0];
}

void msg_inet_network_fn( TrapContext *context )
{
	DPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	ULONG *regs = (ULONG *) context -> regs.regs;
	struct handle_thread_s *thread = GetCurrentThread() ;
	DREG(0) =  thread_inet_network ( (STRPTR) AREG(0) );
}

/* inet_network(cp)(a0) */
static uae_u32 REGPARAM2 bsdsocklib_inet_network (TrapContext *context)
{
	struct socketbase *sb = get_socketbase (context);
	send_thread_msg( sb, context, msg_inet_network_fn );
	return context -> regs.regs[0];
}

#define out(o) DPrintf("%s",o)

void print_hostent(struct hostent *hostent)
{
	char **s;

	out("name:");
	out( hostent -> h_name );
	out("\n");

	if (hostent -> h_aliases)
	{
		out("aliases:");
		for (s= hostent -> h_aliases;*s;s++)
		{
			out(*s); out("\n");
		}
	}
}

void dump_regs(ULONG *regs)
{
	DPrintf("\nD0: %08lx D1: %08lx D2: %08lx D3: %08lx D4: %08lx D5: %08lx D6: %08lx D7: %08lx\n",
		regs[0],regs[1],regs[2],regs[3],regs[4],regs[5],regs[6],regs[7]);

	DPrintf("A0: %08lx A1: %08lx A2: %08lx A3: %08lx A4: %08lx A5: %08lx A6: %08lx A7: %08lx\n\n",
		regs[8],regs[9],regs[10],regs[11],regs[12],regs[13],regs[14],regs[15]);
}

void msg_gethostbyname_fn( TrapContext *context )
{
	DPrintf("%p:%s:%s:%ld\n",FindTask(NULL),__FILE__,__FUNCTION__,__LINE__);

	struct socketbase *sb = get_socketbase (context);
	ULONG *regs = (ULONG *) context -> regs.regs;
	struct handle_thread_s *thread = GetCurrentThread() ;
	struct hostent *hostent;

	hostent =  thread_gethostbyname ( (STRPTR) AREG(0) );

	if (hostent)
	{
		print_hostent( hostent );
		copyHostentToGuest ( context, hostent, sb );
		m68k_dreg (&context->regs, 0) = (uae_u32) sb -> hostent;
	}
	else
	{
		m68k_dreg (&context->regs, 0) = 0;	// on fail return a NULL pointer...
	}
}

/* *------ gethostbyname etc */
/* gethostbyname(name)(a0) */
static uae_u32 REGPARAM2 bsdsocklib_gethostbyname (TrapContext *context)
{
	struct socketbase *sb = get_socketbase (context);
	send_thread_msg( sb, context, msg_gethostbyname_fn );
	return m68k_dreg (&context->regs, 0);
}

void msg_gethostbyaddr_fn( TrapContext *context )
{
	DPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	ULONG *regs = (ULONG *) context -> regs.regs;
	struct handle_thread_s *thread = GetCurrentThread() ;
	struct hostent *hostent;

	hostent =  thread_gethostbyaddr ( (char *) AREG(0), DREG(0), DREG(1) );
	m68k_areg (&context->regs, 0) = (uae_u32) hostent;
}

/* gethostbyaddr(addr, len, type)(a0/d0/d1) */
static uae_u32 REGPARAM2 bsdsocklib_gethostbyaddr (TrapContext *context)
{
	struct socketbase *sb = get_socketbase (context);
	send_thread_msg( sb, context, msg_gethostbyaddr_fn );
	return context -> regs.regs[0];
}

/* getnetbyname(name)(a0) */
static uae_u32 REGPARAM2 bsdsocklib_getnetbyname (TrapContext *context)
{
	DPrintf("%p:%s:%s:%ld\n",FindTask(NULL),__FILE__,__FUNCTION__,__LINE__);
	write_log ("bsdsocket: UNSUPPORTED: getnetbyname()\n");
	return 0;
}

/* getnetbyaddr(net, type)(d0/d1) */
static uae_u32 REGPARAM2 bsdsocklib_getnetbyaddr (TrapContext *context)
{
	DPrintf("%p:%s:%s:%ld\n",FindTask(NULL),__FILE__,__FUNCTION__,__LINE__);
	write_log ("bsdsocket: UNSUPPORTED: getnetbyaddr()\n");
	return 0;
}

void msg_getservbyname_fn( TrapContext *context )
{
	DPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	ULONG *regs = (ULONG *) context -> regs.regs;
	struct handle_thread_s *thread = GetCurrentThread() ;
	struct hostent *hostent;
	hostent =  thread_gethostbyaddr ( (char *) AREG(0), DREG(0), DREG(1) );
	m68k_areg (&context->regs, 0) = (uae_u32) hostent;
}

/* getservbyname(name, proto)(a0/a1) */
static uae_u32 REGPARAM2 bsdsocklib_getservbyname (TrapContext *context)
{
	struct socketbase *sb = get_socketbase (context);
	send_thread_msg( sb, context, msg_getservbyname_fn );
	return context -> regs.regs[0];
}

void msg_getservbyport_fn( TrapContext *context )
{
	DPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	ULONG *regs = (ULONG *) context -> regs.regs;
	struct handle_thread_s *thread = GetCurrentThread() ;
	struct servent *servent;

	servent =  thread_getservbyport( DREG(0), (STRPTR) AREG(0) );
	m68k_areg (&context->regs, 0) = (uae_u32) servent;
}

/* getservbyport(port, proto)(d0/a0) */
static uae_u32 REGPARAM2 bsdsocklib_getservbyport (TrapContext *context)
{
	struct socketbase *sb = get_socketbase (context);
	send_thread_msg( sb, context, msg_getservbyport_fn );
	return context -> regs.regs[0];
}

void msg_getprotobyname_fn( TrapContext *context )
{
	DPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	ULONG *regs = (ULONG *) context -> regs.regs;
	struct handle_thread_s *thread = GetCurrentThread() ;
	struct socketbase *sb = get_socketbase (context);
	struct protoent *protoent;
	protoent =  thread_getprotobyname( (STRPTR) AREG(0) );

	copyProtoentToGuest( context, protoent, sb );
	m68k_areg (&context->regs, 0) = (uae_u32) protoent;
}

/* getprotobyname(name)(a0) */
static uae_u32 REGPARAM2 bsdsocklib_getprotobyname (TrapContext *context)
{
	struct socketbase *sb = get_socketbase (context);
	send_thread_msg( sb, context, msg_getprotobyname_fn );
	return context -> regs.regs[0];
}

void msg_getprotobynumber_fn( TrapContext *context )
{
	DPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	ULONG *regs = (ULONG *) context -> regs.regs;
	struct handle_thread_s *thread = GetCurrentThread() ;
	struct socketbase *sb = get_socketbase (context);

	struct protoent *protoent;
	protoent =  thread_getprotobynumber ( DREG(0) );

	copyProtoentToGuest( context, protoent, sb );
	m68k_areg (&context->regs, 0) = (uae_u32) protoent;
}

/* getprotobynumber(proto)(d0)  */
static uae_u32 REGPARAM2 bsdsocklib_getprotobynumber (TrapContext *context)
{
	struct socketbase *sb = get_socketbase (context);
	send_thread_msg( sb, context, msg_getprotobynumber_fn );
	return context -> regs.regs[0];
}

/* *------ syslog functions */
/* Syslog(level, format, ap)(d0/a0/a1) */
static uae_u32 REGPARAM2 bsdsocklib_vsyslog (TrapContext *context)
{
	write_log ("bsdsocket: UNSUPPORTED: vsyslog()\n");
	return 0;
}

void msg_Dup2Socket_fn( TrapContext *context )
{
	DPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	ULONG *regs = (ULONG *) context -> regs.regs;
	struct handle_thread_s *thread = GetCurrentThread() ;
	DREG(0) =  thread_Dup2Socket ( DREG(0),DREG(1) );
}

/* *------ AmiTCP/IP 1.1 extensions */
/* Dup2Socket(fd1, fd2)(d0/d1) */
static uae_u32 REGPARAM2 bsdsocklib_Dup2Socket (TrapContext *context)
{
	struct socketbase *sb = get_socketbase (context);

	DPrintf("%p:%s:%s:%ld\n",FindTask(NULL),__FILE__,__FUNCTION__,__LINE__);
	send_thread_msg( sb, context, msg_Dup2Socket_fn );
	return context -> regs.regs[0];
}

static uae_u32 REGPARAM2 bsdsocklib_sendmsg (TrapContext *context)
{
	write_log ("bsdsocket: UNSUPPORTED: sendmsg()\n");
	return 0;
}

static uae_u32 REGPARAM2 bsdsocklib_recvmsg (TrapContext *context)
{
	write_log ("bsdsocket: UNSUPPORTED: recvmsg()\n");
	return 0;
}

void msg_gethostname_fn( TrapContext *context )
{
	DPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	ULONG *regs = (ULONG *) context -> regs.regs;
	struct handle_thread_s *thread = GetCurrentThread() ;
	DREG(0) =  thread_gethostname ( (STRPTR) AREG(0), DREG(0) );
}

static uae_u32 REGPARAM2 bsdsocklib_gethostname (TrapContext *context)
{
	DPrintf("%p:%s:%s:%ld\n",FindTask(NULL),__FILE__,__FUNCTION__,__LINE__);

	struct socketbase *sb = get_socketbase (context);	
	send_thread_msg( sb, context, msg_gethostname_fn );
	return context -> regs.regs[0];
}

static uae_u32 REGPARAM2 bsdsocklib_gethostid (TrapContext *context)
{
	write_log ("bsdsocket: WARNING: Process '%s' calls deprecated function gethostid() - returning 127.0.0.1\n", get_real_address (get_long (gettask (context) + 10)));
	return 0x7f000001;
}

const char * const errortexts[] =
{"No error", "Operation not permitted", "No such file or directory",
 "No such process", "Interrupted system call", "Input/output error", "Device not configured",
 "Argument list too long", "Exec format error", "Bad file descriptor", "No child processes",
 "Resource deadlock avoided", "Cannot allocate memory", "Permission denied", "Bad address",
 "Block device required", "Device busy", "Object exists", "Cross-device link",
 "Operation not supported by device", "Not a directory", "Is a directory", "Invalid argument",
 "Too many open files in system", "Too many open files", "Inappropriate ioctl for device",
 "Text file busy", "File too large", "No space left on device", "Illegal seek",
 "Read-only file system", "Too many links", "Broken pipe", "Numerical argument out of domain",
 "Result too large", "Resource temporarily unavailable", "Operation now in progress",
 "Operation already in progress", "Socket operation on non-socket", "Destination address required",
 "Message too long", "Protocol wrong type for socket", "Protocol not available",
 "Protocol not supported", "Socket type not supported", "Operation not supported",
 "Protocol family not supported", "Address family not supported by protocol family",
 "Address already in use", "Can't assign requested address", "Network is down",
 "Network is unreachable", "Network dropped connection on reset", "Software caused connection abort",
 "Connection reset by peer", "No buffer space available", "Socket is already connected",
 "Socket is not connected", "Can't send after socket shutdown", "Too many references: can't splice",
 "Connection timed out", "Connection refused", "Too many levels of symbolic links",
 "File name too long", "Host is down", "No route to host", "Directory not empty",
 "Too many processes", "Too many users", "Disc quota exceeded", "Stale NFS file handle",
 "Too many levels of remote in path", "RPC struct is bad", "RPC version wrong",
 "RPC prog. not avail", "Program version wrong", "Bad procedure for program", "No locks available",
 "Function not implemented", "Inappropriate file type or format", "PError 0"};

uae_u32 errnotextptrs[sizeof (errortexts) / sizeof (*errortexts)];
uae_u32 number_sys_error = sizeof (errortexts) / sizeof (*errortexts);


const char * const herrortexts[] =
 {"No error", "Unknown host", "Host name lookup failure", "Unknown server error",
 "No address associated with name"};

uae_u32 herrnotextptrs[sizeof (herrortexts) / sizeof (*herrortexts)];
uae_u32 number_host_error = sizeof (herrortexts) / sizeof (*herrortexts);

static const char * const strErr = "Errlist lookup error";
uae_u32 strErrptr;

#if 0

#define TAG_DONE   (0L)		/* terminates array of TagItems. ti_Data unused */
#define	TAG_IGNORE (1L)		/* ignore this item, not end of array			   */
#define	TAG_MORE   (2L)		/* ti_Data is pointer to another array of TagItems */
#define	TAG_SKIP   (3L)		/* skip this and the next ti_Data items	 */
#define TAG_USER   ((uae_u32)(1L<<31))

#define SBTF_VAL 0x0000
#define SBTF_REF 0x8000
#define SBTB_CODE 1
#define SBTS_CODE 0x3FFF
#define SBTM_CODE(tag) ((((UWORD)(tag))>>SBTB_CODE) & SBTS_CODE)
#define SBTF_GET  0x0
#define SBTF_SET  0x1
#define SBTM_GETREF(code) \
 (TAG_USER | SBTF_REF | (((code) & SBTS_CODE) << SBTB_CODE))
#define SBTM_GETVAL(code) (TAG_USER | (((code) & SBTS_CODE) << SBTB_CODE))
#define SBTM_SETREF(code) \
 (TAG_USER | SBTF_REF | (((code) & SBTS_CODE) << SBTB_CODE) | SBTF_SET)
#define SBTM_SETVAL(code) \
 (TAG_USER | (((code) & SBTS_CODE) << SBTB_CODE) | SBTF_SET)
#define SBTC_BREAKMASK	  1
#define SBTC_SIGIOMASK	  2
#define SBTC_SIGURGMASK	 3
#define SBTC_SIGEVENTMASK   4
#define SBTC_ERRNO		  6
#define SBTC_HERRNO		 7
#define SBTC_DTABLESIZE	 8
#define SBTC_FDCALLBACK	 9
#define SBTC_LOGSTAT		10
#define SBTC_LOGTAGPTR	  11
#define SBTC_LOGFACILITY	12
#define SBTC_LOGMASK		13
#define SBTC_ERRNOSTRPTR	14	/* <sys/errno.h> */
#define SBTC_HERRNOSTRPTR   15	/* <netdb.h> */
#define SBTC_IOERRNOSTRPTR  16	/* <exec/errors.h> */
#define SBTC_S2ERRNOSTRPTR  17	/* <devices/sana2.h> */
#define SBTC_S2WERRNOSTRPTR 18	/* <devices/sana2.h> */
#define SBTC_ERRNOBYTEPTR   21
#define SBTC_ERRNOWORDPTR   22
#define SBTC_ERRNOLONGPTR   24
#define SBTC_HERRNOLONGPTR  25
#define SBTC_RELEASESTRPTR  29

#endif
static void tagcopy (uae_u32 currtag, uae_u32 currval, uae_u32 tagptr, uae_u32 * ptr)
{
	DPrintf("%p:%s:%s:%ld\n",FindTask(NULL),__FILE__,__FUNCTION__,__LINE__);

	switch (currtag & 0x8001)
	{
		 case 0x0000:		/* SBTM_GETVAL */
			put_long (tagptr + 4, *ptr);
			break;

		 case 0x8000:		/* SBTM_GETREF */
			put_long (currval, *ptr);
			break;

		 case 0x0001:		/* SBTM_SETVAL */
			*ptr = currval;
			break;

		default:			/* SBTM_SETREF */
			*ptr = get_long (currval);
	}
}


// New: Handle SocketBaseTagList trap
void msg_SocketBaseTagList_fn(TrapContext *context)
{
	DPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	ULONG *regs = (ULONG *) context -> regs.regs;
	struct socketbase *sb = get_socketbase (context);
	struct handle_thread_s *thread = (struct handle_thread_s *)GetCurrentThread();
	struct TagItem *guest_tags = (struct TagItem *) guest_AREG(0);
	struct TagItem *host_tags = NULL;

	if (!guest_tags)
	{
		DREG(0) = 0;
		return;
	}
	else
	{
		guest_tags = (struct TagItem *) get_real_address( (uae_u32) guest_tags );
	}

	DPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	// Count tags
	int count = 0;
	for (struct TagItem *t = guest_tags; t->ti_Tag != TAG_DONE; t++)
		count++;

	DPrintf("count: %ld\n",count);

	DPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	host_tags = AllocVecTags(
					sizeof(struct TagItem) * (count + 1), 
					AVT_ClearWithValue, 0, 
					TAG_END);

	if (!host_tags)
	{
		DREG(0) = 0;
		return;
	}

DPrintf("SBTM_GETREF(SBTC_ERRNOSTRPTR) is: %08lx\n",SBTM_GETREF(SBTC_ERRNOSTRPTR));

	for (int i = 0; i < count; i++)
	{
		host_tags[i].ti_Tag = guest_tags[i].ti_Tag;
		host_tags[i].ti_Data = guest_tags[i].ti_Data;

		// Special case: convert pointers from guest to host
		switch (host_tags[i].ti_Tag)
		{
			case SBTM_SETVAL(SBTC_ERRNOPTR(4) ):

				DPrintf("tag SBTM_SETVAL(SBTC_ERRNOPTR(4)) data %08lx\n", host_tags[i].ti_Data);

				sb->errnoptr = (uae_u32) guest_tags[i].ti_Data;
				host_tags[i].ti_Data = (ULONG)get_real_address(host_tags[i].ti_Data);
				break;


			case SBTM_SETVAL(SBTC_LOGTAGPTR):

				DPrintf("tag SBTM_SETVAL(SBTC_LOGTAGPTR) data %08lx\n", host_tags[i].ti_Data);

				sb->logtagptr = (uae_u32) guest_tags[i].ti_Data;
				host_tags[i].ti_Data = (ULONG)get_real_address(host_tags[i].ti_Data);
				break;

/*
			case SBTM_SETVAL(SBTC_TASK):
			case SBTM_SETVAL(SBTC_COOKIE(4) ):
			case SBTM_SETVAL(SBTC_LIBPRIVATE(4) ):
				host_tags[i].ti_Data = (ULONG)get_real_address(host_tags[i].ti_Data);
				break;
*/

			case SBTM_GETVAL(SBTC_ERRNOPTR(4)):

				DPrintf("tag SBTM_GETVAL(SBTC_ERRNOPTR(4)) data %08lx\n", host_tags[i].ti_Data);

				host_tags[i].ti_Data = (ULONG)get_real_address(host_tags[i].ti_Data);
				host_tags[i].ti_Tag = TAG_IGNORE; // should not be handled by native SocketBaseTagList

				// find guest address in the socketbase insted...
				*( (uae_u32 *) host_tags[i].ti_Data) = sb->errnoptr;
				break;

			case SBTM_GETVAL(SBTC_LOGTAGPTR):

				DPrintf("tag SBTM_GETVAL(SBTC_LOGTAGPTR) data %08lx\n", host_tags[i].ti_Data);

				host_tags[i].ti_Data = (ULONG)get_real_address(host_tags[i].ti_Data);
				host_tags[i].ti_Tag = TAG_IGNORE; // should not be handled by native SocketBaseTagList
				*( (uae_u32 *) host_tags[i].ti_Data) = sb->logtagptr;
				break;

			case SBTM_GETREF(SBTC_ERRNOSTRPTR):
				DPrintf("tag SBTM_GETREF(SBTC_ERRNOSTRPTR) data %08lx\n", host_tags[i].ti_Data);
				host_tags[i].ti_Data = (ULONG)get_real_address(host_tags[i].ti_Data);
				break;

			case SBTM_GETREF(SBTC_HAVE_ROADSHOWDATA_API):
				DPrintf("tag SBTM_GETREF(SBTC_HAVE_ROADSHOWDATA_API) data %08lx\n", host_tags[i].ti_Data);
				host_tags[i].ti_Data = (ULONG)get_real_address(host_tags[i].ti_Data);
				break;

/*
			case SBTM_GETVAL(SBTC_COOKIE):
			case SBTM_GETVAL(SBTC_LIBPRIVATE):
				host_tags[i].ti_Data = (ULONG)get_real_address(host_tags[i].ti_Data);
				break;

			case SBTM_GETVAL(SBTC_TASK):
				DPrintf("[SocketBaseTagList] WARNING: skipping guest TASK ptr conversion\n");
				host_tags[i].ti_Data = 0;
				break;
*/

			default:
				DPrintf("tag %08lx data %08lx\n", host_tags[i].ti_Tag, host_tags[i].ti_Data);
				break;

		}


	}
	
	host_tags[count].ti_Tag = TAG_DONE;
	host_tags[count].ti_Data = 0;

	thread_SocketBaseTagList(host_tags);

	FreeVec(host_tags);
	DREG(0) = 0;
}


static uae_u32 REGPARAM2 bsdsocklib_SocketBaseTagList (TrapContext *context)
{
	DPrintf("%p:%s:%s:%ld\n",FindTask(NULL),__FILE__,__FUNCTION__,__LINE__);

	struct socketbase *sb = get_socketbase (context);	
	send_thread_msg( sb, context, msg_SocketBaseTagList_fn );
	return context -> regs.regs[0];
}

static uae_u32 REGPARAM2 bsdsocklib_GetSocketEvents (TrapContext *context)
{
	DPrintf("%p:%s:%s:%ld\n",FindTask(NULL),__FILE__,__FUNCTION__,__LINE__);

#ifdef _WIN32
	struct socketbase *sb = get_socketbase (context);
	int i;
	int flags;
	uae_u32 ptr = m68k_areg (&context->regs, 0);

	TRACE (("GetSocketEvents(0x%x) -> ", ptr));

	for (i = sb->dtablesize; i--; sb->eventindex++) {
	if (sb->eventindex >= sb->dtablesize)
		sb->eventindex = 0;

	if (sb->mtable[sb->eventindex]) {
		flags = sb->ftable[sb->eventindex] & SET_ALL;
		if (flags) {
		sb->ftable[sb->eventindex] &= ~SET_ALL;
		put_long (m68k_areg (&context->regs, 0), flags >> 8);
		TRACE (("%d (0x%x)\n", sb->eventindex + 1, flags >> 8));
		return sb->eventindex; // xxx
		}
	}
	}
#endif
	TRACE (("-1\n"));
	return -1;
}

static uae_u32 REGPARAM2 bsdsocklib_getdtablesize (TrapContext *context)
{
	DPrintf("%p:%s:%s:%ld\n",FindTask(NULL),__FILE__,__FUNCTION__,__LINE__);

	return get_socketbase (context)->dtablesize;
}

static uae_u32 REGPARAM2 bsdsocklib_null (TrapContext *context)
{
	DPrintf("%p:%s:%s:%ld\n",FindTask(NULL),__FILE__,__FUNCTION__,__LINE__);

	return 0;
}

static uae_u32 REGPARAM2 bsdsocklib_init (TrapContext *context)
{
	uae_u32 tmp1;
	int i;
	write_log ("Creating UAE bsdsocket.library 4.1\n");
	if (SockLibBase)
	bsdlib_reset ();

	DPrintf("%p:%s:%s:%ld\n",FindTask(NULL),__FILE__,__FUNCTION__,__LINE__);

	m68k_areg (&context->regs, 0) = functable;
	m68k_areg (&context->regs, 1) = datatable;
	m68k_areg (&context->regs, 2) = 0;
	m68k_dreg (&context->regs, 0) = LIBRARY_SIZEOF;
	m68k_dreg (&context->regs, 1) = 0;
	tmp1 = CallLib (context, m68k_areg (&context->regs, 6), -0x54);

	if (!tmp1) {
		write_log ("bsdoscket: FATAL: Cannot create bsdsocket.library!\n");
		return 0;
	}

	m68k_areg (&context->regs, 1) = tmp1;
	CallLib (context, m68k_areg (&context->regs, 6), -0x18c);
	SockLibBase = tmp1;
#if 0
	m68k_areg (&context->regs, 1) = ds ("dos.library");
	m68k_dreg (&context->regs, 0) = 0;
	dosbase = CallLib (context, m68k_areg (&context->regs, 6), -552);
	printf ("%08lx\n", dosbase);
#endif

	/* Install error strings in Amiga memory */
	tmp1 = 0;

	for (i = number_sys_error; i--;)
	tmp1 += strlen (errortexts[i]) + 1;

	for (i = number_host_error; i--;)
	tmp1 += strlen (herrortexts[i]) + 1;

	tmp1 += strlen(strErr) + 1;

	tmp1 = guest_AllocMem( context, tmp1, 0 );

	if (!tmp1)
	{
		write_log ("bsdsocket: FATAL: Ran out of memory while creating bsdsocket.library!\n");
		return 0;
	}

	for (i = 0; i < (int) (number_sys_error); i++)
	errnotextptrs[i] = addstr (&tmp1, errortexts[i]);

	for (i = 0; i < (int) (number_host_error); i++)
	herrnotextptrs[i] = addstr (&tmp1, herrortexts[i]);

	strErrptr = addstr (&tmp1, strErr);

	/* @@@ someone please implement a proper interrupt handler setup here :) */
	tmp1 = here ();

//	calltrap (deftrap2 (bsdsock_int_handler, TRAPFLAG_EXTRA_STACK | TRAPFLAG_NO_RETVAL, "bsdsock_int_handler"));

	dw (0x4ef9);
	dl (get_long (context->regs.vbr + 0x78));
	put_long (context->regs.vbr + 0x78, tmp1);

	m68k_dreg (&context->regs, 0) = 1;
	return 0;
}

void 	bsdlib_reset ()
{
//	DPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
}

void socketbase_cleanup(SB)
{
//	DPrintf("%p:%s:%s:%ld\n",FindTask(NULL),__FILE__,__FUNCTION__,__LINE__);
}

void 	socketbase_reset (void)
{
//	DPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	struct socketbase sb;
	int s;

	for (s=0;s<MaxSockets;s++)
	{
		if (sockets[s].ProxyPort)
		{
			sb.thread_id = s;
			sb.ProxyPort = sockets[s].ProxyPort;
			send_thread_msg( &sb, NULL, msg_kill_thread_fn );
		}
	}
}

void bsdsocket_os41_reset (void)
{
	SB, *nsb;
	int i;

//	DPrintf("%p:%s:%s:%ld\n",FindTask(NULL),__FILE__,__FUNCTION__,__LINE__);

	if (currprefs.socket_emu == 0)
	return;

	SockLibBase = 0;

	for (sb = socketbases; sb; sb = nsb)
	{
		nsb = sb->next;
		socketbase_cleanup (sb);
		if (sb->dtable)
		{
			free (sb->dtable);
			sb->dtable = NULL;
		}

		if (sb->ftable)
		{
			free (sb->ftable);
			sb->ftable = NULL;
		}
		free (sb);
	}

	socketbases = NULL;
	sbsigqueue = NULL;

	socketbase_reset ();
}

static const TrapHandler sockfuncs[] = {
	bsdsocklib_init, bsdsocklib_Open, bsdsocklib_Close, bsdsocklib_Expunge,
	bsdsocklib_socket, bsdsocklib_bind, bsdsocklib_listen, bsdsocklib_accept,
	bsdsocklib_connect, bsdsocklib_sendto, bsdsocklib_send, bsdsocklib_recvfrom, bsdsocklib_recv,
	bsdsocklib_shutdown, bsdsocklib_setsockopt, bsdsocklib_getsockopt, bsdsocklib_getsockname,
	bsdsocklib_getpeername, bsdsocklib_IoctlSocket, bsdsocklib_CloseSocket, bsdsocklib_WaitSelect,
	bsdsocklib_SetSocketSignals, bsdsocklib_getdtablesize, bsdsocklib_ObtainSocket, bsdsocklib_ReleaseSocket,
	bsdsocklib_ReleaseCopyOfSocket, bsdsocklib_Errno, bsdsocklib_SetErrnoPtr, bsdsocklib_Inet_NtoA,
	bsdsocklib_inet_addr, bsdsocklib_Inet_LnaOf, bsdsocklib_Inet_NetOf, bsdsocklib_Inet_MakeAddr,
	bsdsocklib_inet_network, bsdsocklib_gethostbyname, bsdsocklib_gethostbyaddr, bsdsocklib_getnetbyname,
	bsdsocklib_getnetbyaddr, bsdsocklib_getservbyname, bsdsocklib_getservbyport, bsdsocklib_getprotobyname,
	bsdsocklib_getprotobynumber, bsdsocklib_vsyslog, bsdsocklib_Dup2Socket, bsdsocklib_sendmsg,
	bsdsocklib_recvmsg, bsdsocklib_gethostname, bsdsocklib_gethostid, bsdsocklib_SocketBaseTagList,
	bsdsocklib_GetSocketEvents
};

static const char * const funcnames[] = {
	"bsdsocklib_init", "bsdsocklib_Open", "bsdsocklib_Close", "bsdsocklib_Expunge",
	"bsdsocklib_socket", "bsdsocklib_bind", "bsdsocklib_listen", "bsdsocklib_accept",
	"bsdsocklib_connect", "bsdsocklib_sendto", "bsdsocklib_send", "bsdsocklib_recvfrom", "bsdsocklib_recv",
	"bsdsocklib_shutdown", "bsdsocklib_setsockopt", "bsdsocklib_getsockopt", "bsdsocklib_getsockname",
	"bsdsocklib_getpeername", "bsdsocklib_IoctlSocket", "bsdsocklib_CloseSocket", "bsdsocklib_WaitSelect",
	"bsdsocklib_SetSocketSignals", "bsdsocklib_getdtablesize", "bsdsocklib_ObtainSocket", "bsdsocklib_ReleaseSocket",
	"bsdsocklib_ReleaseCopyOfSocket", "bsdsocklib_Errno", "bsdsocklib_SetErrnoPtr", "bsdsocklib_Inet_NtoA",
	"bsdsocklib_inet_addr", "bsdsocklib_Inet_LnaOf", "bsdsocklib_Inet_NetOf", "bsdsocklib_Inet_MakeAddr",
	"bsdsocklib_inet_network", "bsdsocklib_gethostbyname", "bsdsocklib_gethostbyaddr", "bsdsocklib_getnetbyname",
	"bsdsocklib_getnetbyaddr", "bsdsocklib_getservbyname", "bsdsocklib_getservbyport", "bsdsocklib_getprotobyname",
	"bsdsocklib_getprotobynumber", "bsdsocklib_vsyslog", "bsdsocklib_Dup2Socket", "bsdsocklib_sendmsg",
	"bsdsocklib_recvmsg", "bsdsocklib_gethostname", "bsdsocklib_gethostid", "bsdsocklib_SocketBaseTagList",
	"bsdsocklib_GetSocketEvents"
};

static uae_u32 sockfuncvecs[sizeof (sockfuncs) / sizeof (*sockfuncs)];

#define host_addr(addr)  RTAREA_BASE

void bsdlib_install (void)
{
	char buffer[100];
	uae_u32 resname, resid;
	uae_u32 begin, end;
	uae_u32 func_place, data_place, init_place;
	int i;

	if (currprefs.socket_emu == 0)
	{
		DPrintf("bsdsocket_emu is false, bsdsocket.library not loaded\n");
		return;
	}

//	memset (sockpoolids, UNIQUE_ID, sizeof (sockpoolids));

	resname = ds ("bsdsocket.library");
	resid = ds ("UAE bsdsocket.library 4.1");

	begin = here ();
	dw (0x4AFC);		/* RT_MATCHWORD */
	dl (begin);			/* RT_MATCHTAG */
	dl (0);			/* RT_ENDSKIP */
	dw (0x8004);		/* RTF_AUTOINIT, RT_VERSION */
	dw (0x0970);		/* NT_LIBRARY, RT_PRI */
	dl (resname);		/* RT_NAME */
	dl (resid);			/* RT_IDSTRING */
	dl (here () + 4);		/* RT_INIT */
	dl (512);
	func_place = here ();
	dl (0);
	data_place = here ();
	dl (0);
	init_place = here ();
	dl (0);

	for (i = 0; i < (int) (sizeof (sockfuncs) / sizeof (sockfuncs[0])); i++)
	{
		sockfuncvecs[i] = here ();
		calltrap (deftrap2 (sockfuncs[i], TRAPFLAG_EXTRA_STACK, funcnames[i]));
		dw (RTS);
	}

	/* FuncTable */
	functable = here ();
	for (i = 1; i < 4; i++)
	dl (sockfuncvecs[i]);	/* Open / Close / Expunge */
	dl (EXPANSION_nullfunc);	/* Null */
	for (i = 4; i < (int) (sizeof (sockfuncs) / sizeof (sockfuncs[0])); i++)
	dl (sockfuncvecs[i]);
	dl (0xFFFFFFFF);		/* end of table */

	/* DataTable */
	datatable = here ();
	dw (0xE000);		/* INITBYTE */
	dw (0x0008);		/* LN_TYPE */
	dw (0x0900);		/* NT_LIBRARY */
	dw (0xC000);		/* INITLONG */
	dw (0x000A);		/* LN_NAME */
	dl (resname);
	dw (0xE000);		/* INITBYTE */
	dw (0x000E);		/* LIB_FLAGS */
	dw (0x0600);		/* LIBF_SUMUSED | LIBF_CHANGED */
	dw (0xD000);		/* INITWORD */
	dw (0x0014);		/* LIB_VERSION */
	dw (0x0004);
	dw (0xD000);
	dw (0x0016);		/* LIB_REVISION */
	dw (0x0001);
	dw (0xC000);
	dw (0x0018);		/* LIB_IDSTRING */
	dl (resid);
	dl (0x00000000);		/* end of table */

	end = here ();

	org (begin + 6);		/* Load END value */
	dl (end);

	org (data_place);
	dl (datatable);

	org (func_place);
	dl (functable);

	org (init_place);
	dl (*sockfuncvecs);

	sprintf(buffer,"Install resident: bsdlib rom tag at %p to %p, size: %d bytes\n", begin, end, end - begin);
	write_log(buffer);	

	org (end);

}


static void copyHostentToGuest (TrapContext *context, const struct hostent *hostent, SB)
{
	int size = 28;
	int i;
	int numaddr = 0;
	int numaliases = 0;
	uae_u32 aptr;

	if (hostent->h_name != NULL)
	size += strlen(hostent->h_name)+1;

	if (hostent->h_aliases != NULL)
	while (hostent->h_aliases[numaliases])
		size += strlen(hostent->h_aliases[numaliases++]) + 5;

	if (hostent->h_addr_list != NULL) {
	while (hostent->h_addr_list[numaddr])
		numaddr++;
	size += numaddr*(hostent->h_length+4);
	}

	if (sb->hostent)
		guest_FreeMem( context, sb->hostent, sb->hostentsize );

	sb->hostentsize = size;
	sb->hostent = guest_AllocMem( context, size, 0 );

	if ( sb->hostent == 0 )
	{
		DPrintf("ERROR: Failed to allocated hostent mem\n");
		return;
	}

	aptr = sb->hostent + 28 + numaliases * 4 + numaddr * 4;

	// transfer hostent to Amiga memory
	put_long (sb->hostent + 4, sb->hostent + 20);
	put_long (sb->hostent + 8, hostent->h_addrtype);
	put_long (sb->hostent + 12, hostent->h_length);
	put_long (sb->hostent + 16, sb->hostent + 24 + numaliases*4);

	for (i = 0; i < numaliases; i++)
	put_long (sb->hostent + 20 + i * 4, addstr (&aptr, hostent->h_aliases[i]));
	put_long (sb->hostent + 20 + numaliases * 4, 0);

	for (i = 0; i < numaddr; i++) {
	put_long (sb->hostent + 24 + (numaliases + i) * 4,
		addmem (&aptr, hostent->h_addr_list[i], hostent->h_length));
	}
	put_long (sb->hostent + 24 + numaliases * 4 + numaddr * 4, 0);
	put_long (sb->hostent, aptr);
	addstr (&aptr, hostent->h_name);

	TRACE (("OK (%s)\n",hostent->h_name));


 //   bsdsocklib_seterrno (sb,0);

}


/*
 * Copy a protoent object from native space to Amiga space
 */
static void copyProtoentToGuest (TrapContext *context, const struct protoent *p, SB)
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
	guest_FreeMem (context, sb->protoent, sb->protoentsize);
	}

	sb->protoent = guest_AllocMem (context, size, 0);

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


#else /* ! BSDSOCKET */

#error "did not compile?"

void bsdlib_install (void)
{
   return;
}

#endif
