
#include "sysconfig.h"
#include "sysdeps.h"

#include <assert.h>
#include <stddef.h>

#include "options.h"
#include "include/memory.h"
#include "custom.h"

#include "newcpu.h"
#include "autoconf.h"

#include "traps.h"

#include <proto/exec.h>
#include <proto/dos.h>

#undef __USE_INLINE__
#include <proto/bsdsocket.h>
#include "bsdsocket_amigaos41.h"

#include "od-amiga/win32_handle_emu.h"
#include "od-amiga/win32_thread_emu.h"
#include "od-amiga/thread_socket.h"

struct MsgPort *trap_reply_port = NULL;

HANDLE hThreads[MAX_SELECT_THREADS];

int new_guest_socket( void )
{
	int n;
	for (n=0; n<MaxSockets;n++ )
	{
		if ((sockets[n].ProxyPort == NULL)&&(sockets[n].host_socket == 0))
		{
			return n;	// the index is the guest socket id..
		}
	}
	return -1;
}

void free_guest_socket( int n )
{
	if (n>-1)
	{
		sockets[n].host_socket = 0;
		// we don't need to free it, thread will clean it self up...
		sockets[n].ProxyPort = NULL;	
	}
}

int to_guest_sock( struct MsgPort *host_port, int host_socket )
{
	int n;
	for (n=0; n<MaxSockets;n++ )
	{
		if ((sockets[n].ProxyPort == host_port)&&(sockets[n].host_socket == host_socket))
		{
			return n;	// the index is the guest socket id..
		}
	}
	return -1;
}

void send_thread_msg( struct socketbase *sb, TrapContext *context, void (*callback_fn) (TrapContext *context) )
{
//	Printf("%s:%ld\n",__FUNCTION__,__LINE__);

	struct MsgPort *trap_reply_port = AllocSysObject(ASOT_PORT, TAG_END);
	
	if (trap_reply_port)
	{
		struct ThreadMessage *msg = &ThreadMessages [ sb -> thread_id ];

		bzero(&(msg -> base.mn_Node),sizeof(struct Node));
		msg -> base.mn_ReplyPort = NULL;
		msg -> base.mn_Length = sizeof(struct ThreadMessage);
		msg -> context = context;		// guest sends the CPU registers to host..
		msg -> callback_fn = callback_fn;
		msg -> sender = FindTask(NULL);

		PutMsg( sb -> ProxyPort, (struct Message *) &ThreadMessages[sb -> thread_id] );
		Wait( SIGBREAKF_CTRL_D );
	}
}

void socket_proxy_thread(struct handle_thread_s *thread)
{
	int psig;
	ULONG rsig,sigm;

	ULONG host_waitselect_signal = 0;

	struct ThreadMessage *msg;
//	struct handle_thread_s *

	thread = (struct handle_thread_s *) GetCurrentThread( );

	if (thread == NULL)
	{
		printf("wtf..\n");
		return;
	}

	thread -> t.ProxyPort = AllocSysObject(ASOT_PORT, TAG_END);

	psig = 0;

	if (thread -> t.ProxyPort )
		psig |= 1L << (thread -> t.ProxyPort -> mp_SigBit);

	sigm = psig | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D | host_waitselect_signal;

	thread -> t.running = TRUE;
	while ( thread -> t.running )	// wait for socket to be closed.
	{
		rsig = Wait(sigm);

		if (rsig & psig)
		{
			Printf("\ngot ProxyPort message\n");

			msg = (struct ThreadMessage *) GetMsg( thread -> t.ProxyPort );

			if (msg)
			{
				do
				{
					msg -> callback_fn( msg -> context );
					Signal( msg -> sender, SIGBREAKF_CTRL_D );
					ReplyMsg( (struct Message *) msg);

				} while ( (msg = (struct ThreadMessage *) GetMsg( thread -> t.ProxyPort )) );
			}
		}

		if (rsig & SIGBREAKF_CTRL_C)
		{
			Printf("Socket lost?\n");
		}

#if 0
		if (rsig & host_waitselect_signal)
		{
			// can I make my own context?? is it safe?
			TrapContext context;

			Printf("we need to signal waiting... waitselect()...\n");
			Printf("we don't need to know what fd, was trigged, waitslect should find that out\n");

			context.regs[8+0] = guest_task;		// set A0
			contest.regs[0] = guest_waitselect_signal;	// set D0
  			sigs = CallLib (&context, get_long (4), -324); // send a signal to task ?? 
		}
#endif

		if (rsig & SIGBREAKF_CTRL_D)
		{
			Printf("got SIGBREAKF_CTRL_D\n");

			thread -> t.running = FALSE;
		}
	}

	Printf("removing ProxyPort\n");

	if (thread -> t.ProxyPort)
	{
		FreeSysObject(ASOT_PORT,thread -> t.ProxyPort);
		thread -> t.ProxyPort = NULL;
	}
}

