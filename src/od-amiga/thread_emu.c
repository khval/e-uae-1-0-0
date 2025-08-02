
#include <exec/types.h>
#include <exec/tasks.h>
#include <exec/libraries.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#undef __USE_INLINE__
#include <proto/bsdsocket.h>

#include "win32_handle_emu.h"
#include "win32_thread_emu.h"

extern APTR amiga_thread_safe_mx;
extern struct Task *main_task;
extern int socket_thread_triggered_sigbit;

#define TDEBUG 0

#if TDEBUG
#define DPrintf(fmd,...) Printf(fmd, ##__VA_ARGS__)
#else
#define DPrintf(fmd,...)
#endif

struct handle_thread_s *GetCurrentThread( void )
{
	uint32 index = ((uint32) FindTask(NULL) -> tc_UserData);
	return (struct handle_thread_s *) hThreads[ index ].ptr;
}

struct handle_thread_s *new_thread( APTR func, int index)
{
	struct handle_thread_s *thread ;
	APTR new_task = NULL;

	MutexObtain(amiga_thread_safe_mx);

	HANDLE h= new_handle( h_thread );

	if (( thread = (struct handle_thread_s *) h.ptr ))
	{
		char buffer[100];

		sprintf(buffer,"con:100/100/600/300/thread %d",index);

		thread -> base.index = index;
		bzero( &(thread -> t), sizeof(struct thread_s));		// make sure nothing in thread is set.

		thread -> t.func = func;
#if TDEBUG
		thread -> t.output = Open(buffer,MODE_OLDFILE);
#endif
		// so we can find it in thread_start_func...
		hThreads[ index ].ptr = (struct handle_s *) thread;

		new_task = thread -> base.object = CreateNewProcTags( 
			NP_Start, thread_start_func, 
			NP_Output, thread -> t.output,
			NP_Child, FALSE, 
			NP_CloseOutput, FALSE,
			NP_UserData, (APTR) index, 
			NP_FinalCode, thread_final_func,
			NP_Name, "socketbase_thread",
			TAG_END );

		if ( !new_task )
		{
			if (thread -> t.output)
				Close(thread -> t.output);

			thread -> t.output = 0;

			free( thread );
			return thread;
		}
	}

	MutexRelease(amiga_thread_safe_mx);

	return thread;
}

void __thread_start_func__(struct handle_thread_s *thread)
{
	if (thread)
	{
		AllocSignal(SIGBREAKB_CTRL_D);
		thread -> t.SocketBase = OpenLibrary("bsdsocket.library", 4);

		// prepare thread local stuff.
		if (thread -> t.SocketBase) thread -> t.IS = (struct SocketIFace *) GetInterface ( thread -> t.SocketBase, "main", 1, NULL);

		thread -> t.timerPort = AllocSysObjectTags(ASOT_PORT,	TAG_END);
		if (thread -> t.timerPort)
		{
			thread -> t.timerIO = AllocSysObjectTags(ASOT_IOREQUEST,
						ASOIOR_ReplyPort, thread -> t.timerPort,
						ASOIOR_Size, sizeof(struct TimeRequest),
						TAG_END);

			if (thread -> t.timerIO)
			{
				thread -> t.timerOpenError = OpenDevice("timer.device", UNIT_MICROHZ, (struct IORequest *) thread->t.timerIO, 0) ;
			}
		}

		// Run the function..
		if ((thread -> t.IS) && (thread -> t.timerPort) && (thread -> t.timerIO))
		{
			if (thread -> t.func) thread -> t.func( thread );
		}
		else
		{
			DPrintf("new thread: something went wrong...\ndieing in shame.. (maybe...)\n");
		}
	}
	else
	{
		DPrintf("%s() failed, has no thread\n",__FUNCTION__);
	}
}

void thread_start_func()
{
	struct handle_thread_s *thread = GetCurrentThread();

	__thread_start_func__(thread);
}

void thread_final_func()
{
	// makse sure hThreads[] only has valid pointers.

	DPrintf("%s:%ld\n",__FUNCTION__,__LINE__);

	MutexObtain(amiga_thread_safe_mx);

	struct Task *task = FindTask(NULL);
	uint32 index = (uint32) task -> tc_UserData;

	DPrintf("Closing task: %p\n", task);

	struct handle_thread_s *thread = (struct handle_thread_s *) GetCurrentThread();

	DPrintf("%s:%ld -- index: %ld\n",__FUNCTION__,__LINE__,index);

	if (thread)
	{

	DPrintf("%s:%ld\n",__FUNCTION__,__LINE__);

		// lets unhook it early... maybe it helps..
		hThreads[index].ptr = NULL;	 // unhook the thread struct.

		FreeSignal((BYTE) SIGBREAKB_CTRL_D);

		if (thread->t.timerIO)
		{
			if (thread -> t.timerOpenError == 0 )
			{
				// I have noticed timer can crash if it was never used.

				if (thread->t.timer_used)
				{
	    				if (!CheckIO((struct IORequest *)thread->t.timerIO))
   	 				{
					        AbortIO((struct IORequest *)thread->t.timerIO);
					        WaitIO((struct IORequest *)thread->t.timerIO);
	   		 		}
				}

  	  			CloseDevice((struct IORequest *)thread->t.timerIO);
			}

			FreeSysObject(ASOT_IOREQUEST,thread->t.timerIO);
		}

		if (thread->t.timerPort)
			FreeSysObject(ASOT_PORT,thread->t.timerPort);

		if (thread->t.ProxyPort)
			FreeSysObject(ASOT_PORT,thread->t.ProxyPort);

		if (thread-> t.IS)
			DropInterface((struct Interface *) thread->t.IS );

		if (thread -> t.SocketBase)
			CloseLibrary( thread -> t.SocketBase );

	DPrintf("%s:%ld\n",__FUNCTION__,__LINE__);

		if (thread -> t.output) 
		{
			Close( thread -> t.output );
			thread -> t.output = 0;
		}

		// clear memory, in one operation, faster.. maybe a bit more unsafe..
		bzero( &(thread->t), sizeof(struct thread_s) );

		FreeVec(thread); 	// free the thread struct..
	}

	MutexRelease(amiga_thread_safe_mx);
	hThreads[index].lock = 0;
}


BOOL is_in_hThreads( struct Task *task )
{
	struct handle_thread_s *t;
	uint32 i;

	MutexObtain(amiga_thread_safe_mx);
	for (i = 0; i<MAX_SELECT_THREADS;i++)
	{
		t = (struct handle_thread_s *) hThreads[i].ptr;

		if (t)	if (t -> base.object == task) 

		MutexRelease(amiga_thread_safe_mx);
		return TRUE;
	}

	MutexRelease(amiga_thread_safe_mx);
	return FALSE;
}

int find_new_thread_id()
{
	uint32 i,r=-1;

	MutexObtain(amiga_thread_safe_mx);
	for (i = 0; i<MAX_SELECT_THREADS;i++)
	{
		if ( (hThreads[i].ptr == NULL) && (hThreads[i].lock == 0) )
		{
			hThreads[i].lock = 1;
			r = i;
			break;
		}
	}
	MutexRelease(amiga_thread_safe_mx);

	return r;
}

