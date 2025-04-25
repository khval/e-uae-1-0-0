

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

HANDLE GetCurrentThread( void )
{
	uint32 index = ((uint32) FindTask(NULL) -> tc_UserData);

	Printf("%s: index = %d\n", __FUNCTION__, index);

	if ( hThreads[ index ] ) return hThreads[ index ];

	Printf("task: %s has no thread struct\n", FindTask(NULL) -> tc_Node.ln_Name);

	return NULL;
}

HANDLE new_thread( APTR func, int index)
{
	APTR new_task = NULL;
	BPTR output;

	MutexObtain(amiga_thread_safe_mx);
	struct handle_thread_s *thread = (struct handle_thread_s *) new_handle( h_thread );

	if (thread)
	{
		thread -> BaseClass.index = index;
		thread -> func = func;

		// must be set up by thread_start_func..
		thread -> timerPort = NULL;
		thread -> timerIO = NULL;

		// so we can find it in thread_start_func...
		hThreads[ index ] = (HANDLE) thread;

		output = Open("CON:",MODE_OLDFILE);

		new_task = thread -> BaseClass.object = CreateNewProcTags( 
			NP_Start, thread_start_func, 
			NP_Child, TRUE, 
			NP_Output, output,
			NP_UserData, (APTR) index, 
			NP_FinalCode, thread_final_func,
			NP_Name, "win32_thread_emu",
			TAG_END );

		if (!new_task)
		{
			free( thread);
			return NULL;
		}
	}

	MutexRelease(amiga_thread_safe_mx);

	Printf("%p = new_thread(func: %p,index: %d)\n", thread, func, index);

	return (HANDLE) thread;
}

void __thread_start_func__(HANDLE pHandel)
{
	struct handle_thread_s *thread = (struct handle_thread_s *) pHandel;

	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	if (thread)
	{
		thread -> SocketBase = OpenLibrary("bsdsocket.library", 4);

		// prepare thread local stuff.
		if (thread -> SocketBase) thread -> IS = (struct SocketIFace *) GetInterface ( thread -> SocketBase, "main", 1, NULL);

		thread -> timerPort = AllocSysObjectTags(ASOT_PORT,	TAG_END);

		AllocSignal(SIGBREAKF_CTRL_D);

		if (thread -> timerPort)
		{
			thread -> timerIO = AllocSysObjectTags(ASOT_IOREQUEST,
						ASOIOR_ReplyPort, thread -> timerPort,
						ASOIOR_Size, sizeof(struct TimeRequest),
						TAG_END);

			if (thread -> timerIO)
			{
				thread -> timer = (OpenDevice("timer.device", UNIT_MICROHZ, (struct IORequest *)thread->timerIO, 0) == 0);
			}
		}

		// Run the function..
		if ((thread -> IS) && (thread -> timerPort) && (thread -> timerIO))
		{
			if (thread -> func) thread -> func( thread -> BaseClass.index );
		}
		else
		{
			Printf("new thread: something went wrong...\ndieing in shame.. (maybe...)\n");
		}
	}
	else
	{
		Printf("%s() failed, has no thread\n",__FUNCTION__);
	}
}

void thread_start_func()
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	HANDLE thread = GetCurrentThread();
	__thread_start_func__(thread);
}

void thread_final_func()
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	Delay(20);

	// makse sure hThreads[] only has valid pointers.

	struct Task *task = FindTask(NULL);
	struct handle_thread_s *thread = (struct handle_thread_s *) GetCurrentThread();

//	MutexObtain(amiga_thread_safe_mx);
	
	if (thread)
	{
		FreeSignal(SIGBREAKF_CTRL_D);

		if (thread->timerIO)
		{
			if (thread -> timer)
			{
	    			if (!CheckIO((struct IORequest *)thread->timerIO))
   	 			{
				        AbortIO((struct IORequest *)thread->timerIO);
				        WaitIO((struct IORequest *)thread->timerIO);
	   	 		}

  	  			CloseDevice((struct IORequest *)thread->timerIO);
				thread -> timer = 0;
			}

			FreeSysObject(ASOT_IOREQUEST,thread->timerIO);
   	 		thread->timerIO = NULL;
		}

		if (thread->timerPort)
		{
			FreeSysObject(ASOT_PORT,thread->timerPort);
			thread->timerPort = NULL;
		}

		if (thread-> IS)
		{
			DropInterface((struct Interface *) thread->IS );
			thread->IS = NULL;
		}

		if (thread -> SocketBase)
		{
			CloseLibrary( thread -> SocketBase );
			thread -> SocketBase = NULL;
		}

		thread->BaseClass.object = NULL;	// unassign task..

		hThreads[((uint32) task -> tc_UserData)] = NULL;	 // unhook the thread struct.
		free(thread); 	// free the thread struct..
	}

	// Send a death signal...
	trigger_thread_event( (uint32) task -> tc_UserData );

//	MutexRelease(amiga_thread_safe_mx);
}


void SetThreadPriority(HANDLE thread, int pri)
{
	 SetTaskPri(thread -> object, pri);
}

int GetHThread( struct Task *task )
{
	struct handle_thread_s *thread = NULL;

	int i;

	Printf("task: %p\n",task);
	Printf("amiga_thread_safe_mx: %p\n", amiga_thread_safe_mx);

	MutexObtain(amiga_thread_safe_mx);

	for (i = 0; i<MAX_SELECT_THREADS;i++)
	{
		if (hThreads[i] == NULL)
		{
			Printf("found a empty index %d\n",i);

			thread = (struct handle_thread_s *) new_handle( h_thread );

			if (thread)
			{
				thread -> BaseClass.object = task;
				thread -> func = NULL;

				task -> tc_UserData = (void *) i;
				hThreads[i] = (HANDLE) thread;
				MutexRelease(amiga_thread_safe_mx);

				Printf("run __thread_start_func__(hThreads[i])\n");

				__thread_start_func__(hThreads[i]);
				return i;
			}
			else
			{
				Printf("DANGER: failed to allocated thread space\n");
			}
		}
	}

	MutexRelease(amiga_thread_safe_mx);

	return -1;
}

BOOL is_in_hThreads( struct Task *task )
{
	HANDLE h;
	uint32 i;

	for (i = 0; i<MAX_SELECT_THREADS;i++)
	{
		if ((h = hThreads[i]))
		{
			Printf("index %ld has a thread\n", i);

			if (h -> object == task) return TRUE;
		}
	}
	return FALSE;
}

void trigger_thread_event( uint32 bit )
{
Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	thread_signal_mask |= 1L << bit;	
	Signal( main_task, 1L << socket_thread_triggered_sigbit  );
}

