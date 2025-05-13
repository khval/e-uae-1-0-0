

#include <exec/types.h>
#include <exec/tasks.h>
#include <exec/libraries.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <proto/exec.h>

#include "win32_handle_emu.h"

extern APTR amiga_thread_safe_mx;

struct TagItem shared_tags[]={
	AVT_Type, MEMF_SHARED,
	AVT_Alignment, 8,
	TAG_END};

HANDLE new_handle(int type)
{
	int size;
	HANDLE ret;

	ret.lock = 0;

	switch (type)
	{
		case h_thread:
			size = sizeof(struct handle_thread_s);
			ret.ptr = (struct handle_thread_s *) AllocVecTagList( size, shared_tags );
			if (ret.ptr)
			{
				bzero( ret.ptr, size );
			}
			break;

		case h_event:

			size = sizeof(struct handle_event_s);
			ret.ptr = (struct handle_thread_s *) AllocVecTagList( size, shared_tags );
			if (ret.ptr)
			{
				bzero( ret.ptr, size );
				struct handle_event_s *ev = (struct handle_event_s *) ret.ptr;
				NewList( &(ev -> waiting) );
				ev -> mux = AllocSysObjectTags(ASOT_MUTEX, TAG_DONE);
			}
			break;

		default:
			ret.ptr = AllocVecTagList( sizeof(struct handle_s), shared_tags );	
	}

	if (ret.ptr)
	{
		ret.ptr -> object = NULL;
		ret.ptr -> index = 0;	// default..
		ret.ptr -> type = type;
	}
	else
	{
		printf("%s: failed to allocated memory\n",__FUNCTION__);
	}

	return ret;
}

#define INFINITE (~0)

void CloseHandle( HANDLE *h_ptr )
{
printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	MutexObtain(amiga_thread_safe_mx);
	struct handle_s *h = h_ptr -> ptr;
	
	if ( !h )
	{
		MutexRelease(amiga_thread_safe_mx);
		return;
	}

	switch (h -> type)
	{
		case h_signal:
			FreeSignal( (uint32) h -> object );
			break;

		case h_event:
			{
				struct handle_event_s *ev = (struct handle_event_s *) h;

				if (ev->mux)
				{
					FreeSysObject(ASOT_MUTEX,ev -> mux);
					ev -> mux = NULL;
				}

				if (ev->name)
				{
					free(ev->name);
					ev->name = NULL;
				}
			}
			break;

		case h_thread:
			Signal(h->object, SIGBREAKF_CTRL_C);
			break;

		default:
			printf("%s(): failed, type: %d, not supported\n", __FUNCTION__, h -> type );
			break;
	}

	MutexRelease(amiga_thread_safe_mx);
}


