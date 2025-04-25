
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include <exec/types.h>
#include <exec/tasks.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>

#include "win32_handle_emu.h"    // Your own struct handle_s etc
#include "win32_thread_emu.h"    
#include "win32_event_emu.h"

extern struct Task *main_task;
extern ULONG socket_thread_triggered_sigbit;

void add_to_wait_queue( HANDLE h );
void remove_from_wait_queue( HANDLE h );


HANDLE CreateEvent(int lpEventAttributes,bool bManualReset, bool bInitialSate, const char *lpName)
{
Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	struct handle_event_s *event = (struct handle_event_s *) new_handle( h_event );

	if (event)
	{
		event -> name = lpName ? strdup( lpName ) : NULL;
	}

	return (HANDLE) event;
}

bool SetEvent(HANDLE h)
{
	if (h)
	{
		struct Node *node;
		struct Task *t;
		struct handle_event_s *ev = (struct handle_event_s *) h;

		// signal all threads, that's waiting for the event object.

		for (node = GetHead( &(ev -> waiting) ); node != NULL; node = GetSucc(node) )
		{
			t = (struct Task *) node -> ln_Name;
			Signal( t, SIGBREAKF_CTRL_D ) ;
		}
	}
	else
	{
		printf("Citical error in %s, input is a NULL pointer\n",__FUNCTION__);
	}

	return true;
}


ULONG MsgWaitForMultipleObjects( int nCount, HANDLE *pHandlers, bool opt1, bool opt2, uint32 input_opt )
{
	int n;
	DWORD ret= WAIT_FAILED; // default error..

Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	uint32 sig_mask = 0;
	HANDLE h;
	struct MsgPort *port;
	uint32 rsig;

//	ULONG ret = 0; // should return WAIT_OBJECT_0 or WAIT_OBJECT_2, have no idea, why...

	for (n=0;n<nCount;n++)
	{
		h = pHandlers[n];

		switch ( h -> type)
		{
			case h_event:

				printf("IS EVENT\n");
				add_to_wait_queue( h );	// request notify from all the handlers
				break;

			default:
				printf("%s:%d -- did not expect this type of object\n",__FUNCTION__,__LINE__);
				Delay(50);
				break;
		}
	}

	rsig = Wait( SIGBREAKF_CTRL_D );	// wait for notify..

	for (n=0;n<nCount;n++)
	{
		h = pHandlers[n];

		switch ( h -> type)
		{
			case h_event:
				remove_from_wait_queue(  h );
				break;

			default:
				printf("%s:%d -- did not expect this type of object\n",__FUNCTION__,__LINE__);
				Delay(50);
				break;
		}
	}

	if (sig_mask) 
	{
		Wait(sig_mask);		// can maybe get stuck....  SetSignals( ) maybe better, add a loop, and calculate elapsed time..
		return WAIT_OBJECT_0;	// return always object 0 as, its only one tiem in pHandlers... bad code!!
	}
	
	return ret;
}

DWORD WaitForSingleObject( HANDLE h, int32 time_flag )
{
Printf("%s:%s:%ld   handle ptr = %p, time_flag = %ld\n",__FILE__,__FUNCTION__,__LINE__, h, time_flag);

	DWORD ret= WAIT_FAILED; // default error..
	uint32 rsig;

	if (h == NULL)
	{
		return ret;
	}

	switch ( h -> type)
	{
		case h_event:		// wait for a event to come...

			Printf("IS EVENT\n");

			add_to_wait_queue( h );
			rsig = Wait( SIGBREAKF_CTRL_D );
			remove_from_wait_queue(  h );

			if (rsig & SIGF_SINGLE)
			{
				ret = WAIT_OBJECT_0;
			}
			break;

		case h_thread:		// wait for a thread to die??
			{ 
				Printf("IS THREAD\n");

				uint32 bit_mask = 1L << h -> index;

				if (main_task != FindTask(NULL)) Printf("wtf: at %s:%ld\n",__FUNCTION__,__LINE__);

				while ( ! ( thread_signal_mask & bit_mask  ) )
				{
					thread_signal_mask &= ~bit_mask;
					Wait( 1L <<  socket_thread_triggered_sigbit );
				}

				// we have none handled signals, this should prevent dead lock...
				if (thread_signal_mask) Signal( main_task, 1L <<  socket_thread_triggered_sigbit  );

			}
			break;

		default:

			Printf("wtf: at %s:%d (unexpected object, type: %ld)\n",__FUNCTION__,__LINE__, h -> type);
			break;
	}

	return ret;
}

// 

void add_to_wait_queue( HANDLE h )
{
	struct handle_event_s *ev = (struct handle_event_s *) h;
	struct Node *node;

	MutexObtain( ev -> mux );
	node = malloc(sizeof(struct Node *));
	if (node)
	{
		node -> ln_Name = (APTR) FindTask(NULL);
		AddHead( &(ev -> waiting), node );
	}
	MutexRelease( ev -> mux );
}

void remove_from_wait_queue( HANDLE h )
{
	struct handle_event_s *ev = (struct handle_event_s *) h;
	struct Task *task = FindTask(NULL);
	struct Node *node;

	MutexObtain( ev -> mux );
	for (node=GetHead(&(ev -> waiting)); node != NULL; node = GetSucc(node))
	{
		if (node -> ln_Name == (APTR) task )
		{
			Remove(node);
			break;
		}
	}
	MutexRelease( ev -> mux );
}

