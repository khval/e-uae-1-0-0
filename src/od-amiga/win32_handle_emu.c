

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

HANDLE new_handle(int type)
{
	HANDLE ret;

printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (type)
	{
		case h_thread:
			ret = malloc(sizeof(struct handle_thread_s));
			break;

		case h_event:
			ret = malloc(sizeof(struct handle_event_s));
			if (ret)
			{
				struct handle_event_s *ev = (struct handle_event_s *) ret;
				NewList( &(ev -> waiting) );
				ev -> mux = AllocSysObjectTags(ASOT_MUTEX, TAG_DONE);
			}
			break;

		default:
			ret = malloc(sizeof(struct handle_s));	
	}

	if (ret)
	{
		ret -> object = NULL;
		ret -> index = 0;	// default..
		ret -> type = type;

		printf("%s: type=%d, returned=%p\n",__FUNCTION__,type,ret);
	}
	else
	{
		printf("%s: failed to allocated memory\n",__FUNCTION__);
	}

	return ret;
}

#define INFINITE (~0)

void CloseHandle( HANDLE h )
{
printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (h -> type)
	{
		case h_signal:

			FreeSignal( (uint32) h -> object );
			break;

		case h_event:
			MutexObtain(amiga_thread_safe_mx);
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
			MutexRelease(amiga_thread_safe_mx);
			break;

		case h_thread:

			MutexObtain(amiga_thread_safe_mx);
			Signal(h->object, SIGBREAKF_CTRL_C);
			MutexRelease(amiga_thread_safe_mx);
			WaitForSingleObject(h, INFINITE);
			break;
	break;

		default:
			printf("%s(): failed, type: %d, not supported\n", __FUNCTION__, h -> type );
			break;
	}
}


