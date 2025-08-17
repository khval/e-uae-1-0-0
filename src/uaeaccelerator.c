 /*
  * UAE - The Un*x Amiga Emulator
  *
  * dogshit.library emulation machine-independent part
  *
  * Copyright 1997, 1998 Mathias Ortmann
  *
  * Library initialization code (c) Tauno Taipaleenmaki
  */

#ifdef __amigaos4__


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

#include <proto/exec.h>
#include <proto/dos.h>

#include <exec/resident.h>

#include <libraries/mpega.h>
#include <proto/mpega.h>

#define AcceleratorLib

#ifdef AcceleratorLib

extern int main_task_wakeup_sigbit;

void accelerator_install (void);
void accelerator_reset (void);

#define TRACE printf

#define PAT_LoopbackVolume 1

enum
{
	TT_PlayTask = 1000,
	TT_PlaySignal,
	TT_RawInt,
	TT_Mode,
	TT_Frequency,
	TT_RawBuffer,
	TT_BufferSize,
	TT_RawIrqSize
};

#pragma pack(push, 2)

struct AcceleratorBase {
	struct Library lib;
};

struct jmpe
{
	short opcode;
	ULONG addr;
};

struct rt_init
{
	uae_u32 libSize;
	uae_u32 funcTable;
	uae_u32 InitStructTable;
	uae_u32 InitFunction;

} *rt_init_HA; // host_address;

#pragma pack (pop)

struct rawplayback
{
	ULONG playTask ;
	ULONG playSignal ;
	ULONG mode ;
	ULONG frequency ;
	ULONG rawbuffer[2];
	ULONG buffer_size;
};

struct rawplayback rp;
struct Process *ahi_playback_process = NULL;


void dump_jmp_table( uae_u32 addr );
void show_lib_info( uae_u32 libBase );

#define DREG(n) regs[n]
#define AREG(n) (APTR *) get_real_address (regs[n+8])

/* Memory-related helper functions */
STATIC_INLINE void memcpyha (uae_u32 dst, const char *src, int size)
{
	while (size--)
		put_byte (dst++, *src++);
}

/* Get current task */
static uae_u32 gettask (TrapContext *context)
{
	uae_u32 currtask, a1 = m68k_areg (&context->regs, 1);

	m68k_areg (&context->regs, 1) = 0;
	currtask = CallLib (context, get_long (4), -0x126);	/* FindTask */

	m68k_areg (&context->regs, 1) = a1;

	TRACE (("[%s] ", (APTR) get_real_address (get_long (currtask + 10))));
	return currtask;
}

static uae_u32 REGPARAM2 acceleratorlib_Expunge (TrapContext *context)
{
	write_log ("acceleratorlib_Expunge\n") ;
	return 0;
}

static uae_u32 accel_functable, accel_datatable, inittable;

uae_u32 uae_accelerator_base = 0L;

void show_lib_info( uae_u32 libBase )
{
	char buffer[100];
	struct AcceleratorBase*host_base_addr;

	host_base_addr = (struct AcceleratorBase*) get_real_address (libBase);
	if (host_base_addr)
	{
		sprintf(buffer,"LibBase 0x%08x, Name: %s, NegSize: %d, PosSize: %d, Flags: 0x%08x, Sum: %08x\n", 
			libBase, 
			get_real_address ( (uaecptr) host_base_addr -> lib.lib_Node.ln_Name),
			host_base_addr -> lib.lib_NegSize, 
			host_base_addr -> lib.lib_PosSize,
			host_base_addr -> lib.lib_Flags,
			host_base_addr -> lib.lib_Sum );
		write_log(buffer);
	}
}


static uae_u32 REGPARAM2 acceleratorlib_Open (TrapContext *context)
{
	char buffer[100];
	uae_u32 libBase = m68k_areg (&context->regs, 6);
	struct AcceleratorBase *libBase_HA = (struct AcceleratorBase*) get_real_address ( libBase );

	write_log ("acceleratorlib_Open\n") ;
	show_lib_info( libBase );

	libBase_HA -> lib.lib_OpenCnt ++;
	libBase_HA -> lib.lib_Flags &= ~LIBF_DELEXP;

	sprintf(buffer,"acceleratorlib open count: %d\n", libBase_HA -> lib.lib_OpenCnt );
	write_log (buffer) ;

	m68k_dreg (&context->regs, 0) = libBase;
	return libBase;
}

static uae_u32 REGPARAM2 acceleratorlib_Close (TrapContext *context)
{
	char buffer[100];
	uae_u32 libBase = m68k_dreg (&context->regs, 0);
	struct AcceleratorBase *libBase_HA = (struct AcceleratorBase*) get_real_address ( libBase );

	write_log ("acceleratorlib_Close\n") ;
	show_lib_info( libBase );

	libBase_HA -> lib.lib_OpenCnt --;
//	if (libBase_HA -> lib.lib_OpenCnt == 0)

	if ( libBase_HA ->  lib.lib_Flags & LIBF_CHANGED ) write_log ("LIBF_CHANGED\n") ;
	if ( libBase_HA ->  lib.lib_Flags & LIBF_SUMUSED ) write_log ("LIBF_SUMUSED\n") ;
	if ( libBase_HA -> lib.lib_Flags & LIBF_DELEXP ) write_log ("LIBF_DELEXP\n") ;

	libBase_HA -> lib.lib_Flags = 0L;

	sprintf(buffer,"acceleratorlib open count: %d\n", libBase_HA -> lib.lib_OpenCnt );
	write_log (buffer) ;

	m68k_dreg (&context->regs, 0) = 0L;
	return 0L;
}

void ahi_playback_func( void );
struct MsgPort *playback_msg_port = NULL;
struct Message *playback_msg = NULL;

extern struct Task *main_task;

void ahi_playback_func()
{
	char buf[100];
	ULONG rsigs,sigs;
	struct Message *msg;

	write_log("%s: --started\n", __FUNCTION__);

	playback_msg_port = AllocSysObjectTags( ASOT_PORT, TAG_END);

	if (playback_msg_port)
	{
		sigs = SIGBREAKF_CTRL_C;
		sigs |= 1L << playback_msg_port -> mp_SigBit;

		for(;;)
		{
			rsigs = Wait( sigs );
			if (rsigs & SIGBREAKF_CTRL_C) break;
	
			if (rsigs & (1L << playback_msg_port -> mp_SigBit))
			{
				write_log("%s: --got message\n", __FUNCTION__);

				while ((msg = GetMsg(playback_msg_port)))
				{
					ReplyMsg(msg);
				}

				Delay(50);

				uae_Signal (rp.playTask, ((uae_u32) 1) << rp.playSignal); 
			}
		}

		FreeSysObject( ASOT_PORT, playback_msg_port ); 
		playback_msg_port=NULL;
	}

	write_log("%s: --stoped\n", __FUNCTION__);
	Signal(main_task, 1L << main_task_wakeup_sigbit );
}

static uae_u32 REGPARAM2 acceleratorlib_init (TrapContext *context)
{
	char buffer[60];
	struct AcceleratorBase *host_base_addr;
	uae_u32 tmp1;
	uae_u32 d0 = m68k_dreg (&context->regs, 0);
	uae_u32 a1 = m68k_areg (&context->regs, 0);
	uae_u32 sysBase = m68k_areg (&context->regs, 6);

	write_log ("libBase from D0\n");

	dump_jmp_table(  m68k_dreg (&context->regs, 0) );

	write_log ("MakeLibrary accelerator.library 4.1 for EUAE\n");

	//	library = MakeLibrary(vectors, structure, init, dataSize, segList)

	m68k_areg (&context->regs, 0) = accel_functable;
	m68k_areg (&context->regs, 1) = accel_datatable;
	m68k_areg (&context->regs, 2) = 0;
	m68k_dreg (&context->regs, 0) = sizeof(struct AcceleratorBase);
	m68k_dreg (&context->regs, 1) = 0;
	tmp1 = CallLib (context, sysBase, -0x54);	// MakeLibrary

	if (!tmp1)
	{
		write_log ("accelerator.library: FATAL: Cannot create library!\n");
		return 0;
	}

//	dump_jmp_table( tmp1 );
//	show_lib_info( tmp1 );

	m68k_areg (&context->regs, 1) = tmp1;
	CallLib (context, sysBase, -0x18c); // AddLibrary

	if (playback_msg == NULL)
	{
		playback_msg = AllocSysObjectTags( ASOT_MESSAGE, 
			ASOMSG_Size, sizeof(struct Message),
			TAG_END);
	}

	if (ahi_playback_process == NULL)
	{
		ahi_playback_process = CreateNewProcTags( 
			NP_Start, ahi_playback_func, 
			NP_UserData, (APTR) 0, 
			NP_Child, FALSE, 
			NP_Name, "uae ahi sound playback",
			TAG_END );
	}

	m68k_dreg (&context->regs, 0) = 1;
	return 0;
}

/**************************** DEBUG ***********************************/

static uae_u32 REGPARAM2 accelerator_hostPutStr (TrapContext *context)
{
	ULONG *regs = (ULONG *) context -> regs.regs;
	write_log ( (char *) AREG(0)) ;
	m68k_dreg (&context->regs, 0) = 0;
	return 0;
}

/***************************** AUDIO *****************************************/

// A6 library, A0 taglist

static uae_u32 REGPARAM2 accelerator_SetPartTagList (TrapContext *context)
{
	ULONG *regs = (ULONG *) context -> regs.regs;
	struct TagItem *tag,*tags = (struct TagItem *) AREG(0);
	char buff[100];

	write_log ( __FUNCTION__ ) ;
	write_log ( "\n" ) ;

	if (tags != NULL)
	{
		for (tag = tags; tag -> ti_Tag != TAG_END; tag ++)
		{
			sprintf(buff,"Tag: 0x%08X, Data: 0x%08X\n", tag -> ti_Tag, tag -> ti_Data );
			write_log ( buff ) ;
		}
	}

	m68k_dreg (&context->regs, 0) = 0;
	return 0;
}

// A6 library, A0 taglist



int sndout_cnt = 0;

static void dump_pcm_from_quest( ULONG current_buffer,ULONG guest_ptr, int length )
{
	char filename[100];
	FILE *fd;

	if ((guest_ptr)&&(length>0))
	{
		APTR host_addr = get_real_address(guest_ptr);

		snprintf(filename, sizeof(filename)-1, "ram:sndout-buf-%d-cnt-%ld.raw", current_buffer, sndout_cnt++ );
		write_log("filename: %s, guest_addr: 0x%x, host_addr: 0x%x, length: %d\n", filename, guest_ptr, host_addr, length );

		if (( fd = fopen(filename,"wb") ))
		{
			fwrite( host_addr, 1, length, fd );
			fclose(fd);

			write_log ( "dumped %d bytes to %s\n", length, filename ) ;
		}
	}
}

static uae_u32 REGPARAM2 accelerator_RawPlaybackTagList (TrapContext *context)
{
	ULONG *regs = (ULONG *) context -> regs.regs;
	struct TagItem *tag, *tags = (struct TagItem *) AREG(0);
	char buff[100];
	int n;

	write_log ( "%s\n",__FUNCTION__ ) ;

	/* I think we need a task / process... to handle iorequest or mixing into paula audio,
	once one iorequest are done.. singal for more data? */

	if (tags != NULL)
	{
		for (tag = tags; tag -> ti_Tag != TAG_END; tag ++)
		{
			switch (tag -> ti_Tag)
			{
				case TT_PlayTask:
					rp.playTask = tag -> ti_Data;
					sprintf(buff,"TT_PlayTask: 0x%08X\n", tag -> ti_Data );
					break;

				case TT_PlaySignal: 
					rp.playSignal = tag -> ti_Data;
					sprintf(buff,"TT_PlaySignal: 0x%08X\n", tag -> ti_Data );
					break;

				case TT_Mode: 
					rp.mode = tag -> ti_Data;
					sprintf(buff,"TT_Mode: %ld\n", tag -> ti_Data );
					break;

				case TT_Frequency: 
					rp.frequency = tag -> ti_Data;
					sprintf(buff,"TT_Frequency: %ld\n", tag -> ti_Data );
					break;

				case TT_RawBuffer: 
					rp.rawbuffer[0] = tag -> ti_Data;
					sprintf(buff,"TT_RawBuffer: 0x%08X\n", tag -> ti_Data );
					break;

				case TT_BufferSize: 
					rp.buffer_size = tag -> ti_Data;
					sprintf(buff,"TT_BufferSize: %ld\n", tag -> ti_Data );
					break;

				default:
					sprintf(buff,"Tag: 0x%08X, Data: 0x%08X\n", tag -> ti_Tag, tag -> ti_Data );
					break;
			}

			if (buff[0]) write_log ( buff ) ;
		}
	}

	/* this code is more or less a hack... just dump buffer thats has data.. */

	for(n=0;n<2;n++)
	{
		if (rp.rawbuffer[n])
		{
			write_log ( "%s - idx: %d - sending msg\n",__FUNCTION__, n ) ;
			dump_pcm_from_quest( n, rp.rawbuffer[n], rp.buffer_size );
		}
	}

	if ((playback_msg_port)&&(playback_msg))
	{
		write_log ( "%s - sending msg\n",__FUNCTION__ ) ;
		PutMsg( playback_msg_port , playback_msg );
	}
	else
	{
		if (playback_msg_port == NULL)
			write_log ( "%s - playback_msg_port is NULL\n",__FUNCTION__ ) ;

		if (playback_msg == NULL)
			write_log ( "%s - playback_msg is NULL\n",__FUNCTION__ ) ;
	}

	m68k_dreg (&context->regs, 0) = 0;
	return 0;
}


/*******************************************************************************/

#define TOHOSTPTR( type, arg ) if (arg) arg = (type) get_real_address( (uaecptr) arg );

MPEGA_STREAM *stream_to_host( MPEGA_STREAM *guest_mpega_stream, MPEGA_STREAM *host_mpega_stream);
MPEGA_ACCESS *access_to_host( MPEGA_ACCESS *guest_mpega_access, MPEGA_ACCESS *host_mpega_access);
MPEGA_CTRL *ctrl_to_host( MPEGA_CTRL *guest_mpega_ctrl, MPEGA_CTRL *host_mpega_ctrl );
static uae_u32 REGPARAM2 trap_MPEGA_decode_frame(TrapContext *ctx);
				       
MPEGA_STREAM *stream_to_host( MPEGA_STREAM *guest_mpega_stream, MPEGA_STREAM *host_mpega_stream)
{
	memcpy( host_mpega_stream, guest_mpega_stream, sizeof(MPEGA_STREAM) );

	TOHOSTPTR ( void *, host_mpega_stream -> handle);

	return host_mpega_stream;
}

MPEGA_ACCESS *access_to_host( MPEGA_ACCESS *guest_mpega_access, MPEGA_ACCESS *host_mpega_access)
{
	memcpy( host_mpega_access, guest_mpega_access, sizeof(MPEGA_ACCESS) );

	switch ( host_mpega_access -> func )
	{
		case MPEGA_BSFUNC_OPEN:
			TOHOSTPTR( void *, host_mpega_access -> data.open.stream_name );
			break;

		case MPEGA_BSFUNC_CLOSE:
			break;

		case MPEGA_BSFUNC_READ:
			TOHOSTPTR( void *,host_mpega_access -> data.read.buffer );
			break;
	}

	return host_mpega_access;
}

MPEGA_CTRL *ctrl_to_host( MPEGA_CTRL *guest_mpega_ctrl, MPEGA_CTRL *host_mpega_ctrl )
{
	memcpy( host_mpega_ctrl, guest_mpega_ctrl, sizeof(MPEGA_CTRL) );

	TOHOSTPTR( void *, host_mpega_ctrl -> bs_access );

	return host_mpega_ctrl;
}


/*******************************************************************************/

/* AccCopyMem(dest,source,size)(a0/a1/d0) */
static uae_u32 REGPARAM2 accelerator_CopyMem (TrapContext *context)
{
	ULONG *regs = (ULONG *) context -> regs.regs;
	CopyMem(AREG(0),AREG(1),DREG(0));
	return context -> regs.regs[0];
}

/* AccCopyMemQuick(dest,source,size)(a0/a1/d0) */
static uae_u32 REGPARAM2 accelerator_CopyMemQuick (TrapContext *context)
{
	ULONG *regs = (ULONG *) context -> regs.regs;
	CopyMemQuick(AREG(0),AREG(1),DREG(0));
	return context -> regs.regs[0];
}

/* D0: handle, A0: input, A1: output, D1: size */
static uae_u32 REGPARAM2 trap_MPEGA_decode_frame(TrapContext *ctx)
{
    ULONG *regs = (ULONG *) ctx->regs.regs;

	MPEGA_STREAM *mpega_stream = (MPEGA_STREAM *) AREG(0);
	MPEGA_STREAM host_mpega_stream;
	WORD **pcm = (WORD **) AREG(1);
	WORD **host_pcm;

	host_pcm = alloca( sizeof(WORD *) * MPEGA_MAX_CHANNELS );

	// need to convert guest mpega_stream to host mpega_stream
	// need to copy guest pcm to host pcm.

	DREG(0) = (ULONG) MPEGA_decode_frame( stream_to_host( (MPEGA_STREAM *) AREG(0), &host_mpega_stream), host_pcm );
    return DREG(0);
}

/* D0: handle */
static uae_u32 REGPARAM2 trap_MPEGA_time(TrapContext *ctx)
{
	ULONG *regs = (ULONG *) ctx->regs.regs;

	MPEGA_STREAM *mpega_stream = (MPEGA_STREAM *) AREG(0);
	MPEGA_STREAM host_mpega_stream;

	return MPEGA_time( stream_to_host( (MPEGA_STREAM *) AREG(0), &host_mpega_stream), (ULONG *) DREG(0) );
}

/* D0: handle, A0: data buffer, D1: buffer size */
static uae_u32 REGPARAM2 trap_MPEGA_find_sync(TrapContext *ctx)
{
	ULONG *regs = (ULONG *) ctx->regs.regs;

	// need to convert guest mpega_stream to host mpega_stream

	DREG(0) = MPEGA_find_sync( (BYTE *) AREG(0), (LONG) DREG(0) );
	return DREG(0);
}

/* D0: handle, D1: scale value */
static uae_u32 REGPARAM2 trap_MPEGA_scale(TrapContext *ctx)
{
	ULONG *regs = (ULONG *) ctx->regs.regs;
	MPEGA_STREAM host_mpega_stream;

	DREG(0) = MPEGA_scale( stream_to_host( 
		(MPEGA_STREAM *) AREG(0), 
		&host_mpega_stream), 
		DREG(0) );

	return DREG(0);
}


static const TrapHandler accelerator_funcs[] = {
	acceleratorlib_init, 
	acceleratorlib_Open, 
	acceleratorlib_Close, 
	acceleratorlib_Expunge,
	accelerator_CopyMem, 
	accelerator_CopyMemQuick,
	accelerator_hostPutStr,

	accelerator_SetPartTagList,
	accelerator_RawPlaybackTagList,

	trap_MPEGA_decode_frame , // LVO_MPEGA_decode
	trap_MPEGA_time ,   // LVO_MPEGA_time
	trap_MPEGA_find_sync ,
	trap_MPEGA_scale ,
};

static const char * const funcnames[] = {
	"acceleratorlib_init", 
	"acceleratorlib_Open", 
	"acceleratorlib_Close", 
	"acceleratorlib_Expunge",
	"accelerator_CopyMem", 
	"accelerator_CopyMemQuick",
	"accelerator_hostPutStr",

	"accelerator_SetPartTagList",
	"accelerator_RawPlaybackTagList",

	"MPEGA_decode_frame",
	"MPEGA_time",
	"MPEGA_find_sync",
	"MPEGA_scale",
};

static uae_u32 accelerator_funcvecs[sizeof (accelerator_funcs) / sizeof (*accelerator_funcs)];

void dump_jmp_table( uae_u32 addr)
{
	char buffer[60];
	struct jmpe *j;
	int i;

	addr-=6;	// move to first;

	for (i = 1; i < (int) (sizeof (accelerator_funcs) / sizeof (accelerator_funcs[0])); i++)
	{
		j = (struct jmpe *) get_real_address ( addr );

		sprintf( buffer, "%d - %04X %08X\n",
			-i*6,
			j -> opcode,
			j -> addr);

		write_log(buffer);

		addr-=6;
	}
}

extern void rt_add_struct (uae_u32 size); // alloc a struct in the RTAREA_BASE

void accelerator_install (void)
{
	char buffer[100];
	uae_u32 resname, resid;
	uae_u32 begin, end;
	uae_u32 RT_INIT;
	int i;

	struct Resident *res_HA;		// host address

	resname = ds ("accelerator.library");
	resid = ds ("UAE accelerator.library 4.1");

	begin = here ();

	res_HA = (struct Resident *) get_real_address( here () );		// host address
	rt_add_struct( sizeof(struct Resident) );			// move to next location

	RT_INIT = here ();
	rt_init_HA = (struct rt_init *) get_real_address( RT_INIT );		// host address
	rt_add_struct( sizeof(struct rt_init) );			// move to next location

	res_HA -> rt_MatchWord = 0x4AFC;
	res_HA -> rt_MatchTag = (struct Resident *) begin;  
	res_HA -> rt_EndSkip = 0;
	res_HA -> rt_Flags = RTF_AUTOINIT; 
	res_HA -> rt_Version = 4; 
	res_HA -> rt_Type = NT_LIBRARY; 
	res_HA -> rt_Pri = 0x70; 
	res_HA -> rt_Name = (CONST_STRPTR) resname; 
	res_HA -> rt_IdString = (CONST_STRPTR) resid;  
	res_HA -> rt_Init = (APTR) RT_INIT;

	// Generate the trap code..

	for (i = 0; i < (int) (sizeof (accelerator_funcs) / sizeof (accelerator_funcs[0])); i++)
	{
		accelerator_funcvecs[i] = here ();

	 	sprintf(buffer,"Sub Rutine at 0x%08x\n", (ULONG) accelerator_funcvecs[i] );
		write_log(buffer);

		calltrap (deftrap2 (accelerator_funcs[i], TRAPFLAG_EXTRA_STACK, funcnames[i]));
		dw (RTS);
	}

	// assign trap code to FuncTable

	accel_functable = here ();
	for (i = 1; i < 4; i++) dl (accelerator_funcvecs[i]);	/* Open / Close / Expunge */

	dl ( EXPANSION_nullfunc);	/* Null */

	for (i = 4; i < (int) (sizeof (accelerator_funcs) / sizeof (accelerator_funcs[0])); i++)
		dl (accelerator_funcvecs[i]);

	dl (0xFFFFFFFF);		/* end of table */

	/* accel_datatable */
	accel_datatable = here ();	

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
	dw ( res_HA -> rt_Version );
	dw (0xD000);
	dw (0x0016);		/* LIB_REVISION */
	dw (0x0001);
	dw (0xC000);
	dw (0x0018);		/* LIB_IDSTRING */
	dl (resid);
	dl (0x00000000);		/* end of table */

	end = here ();

	res_HA -> rt_EndSkip = (void *) end;

	rt_init_HA -> libSize = sizeof(struct AcceleratorBase);
	rt_init_HA -> funcTable = accel_functable;
	rt_init_HA -> InitStructTable = accel_datatable;
	rt_init_HA -> InitFunction = *accelerator_funcvecs;

	sprintf(buffer,"Install resident: accelerator rom tag at %p to %p, size: %d bytes\n", begin, end, end - begin);
	write_log(buffer);	

}

void accelerator_reset (void)
{
	if (playback_msg)
	{
		FreeSysObject(ASOT_MESSAGE, playback_msg);
		playback_msg = NULL;
	}

	if (ahi_playback_process)
	{
		Signal( (struct Task *) ahi_playback_process, SIGBREAKF_CTRL_C );
		Wait( 1L << main_task_wakeup_sigbit );
	}

	/* I think we can store first aduio into one buffer 0, 
	if buffer 1 is empty singal for 1 more data or somehing like that...
	the playback can empty buffer. or maybe just counter that toggels input */

	rp.rawbuffer[0] = 0;
	rp.rawbuffer[1] = 0;
}

#else /* ! dogshit */

#error oh shit.

void accelerator_install (void)
{
   return;
}

#endif
#endif 