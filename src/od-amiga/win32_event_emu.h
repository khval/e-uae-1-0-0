
#define INFINITE -1

#define WAIT_OBJECT_0 0	// just to get it compiling (might need to be allocated amiga signal)
#define WAIT_OBJECT_1 1	// just to get it compiling (might need to be allocated amiga signal)
#define WAIT_TIMEOUT 0x102
#define WAIT_FAILED (~0)

typedef unsigned short DWORD;

HANDLE CreateEvent(int lpEventAttributes,bool bManualReset, bool bInitialSate, const char *lpName);
bool SetEvent(HANDLE hEvent);
DWORD WaitForSingleObject( HANDLE h, int32 time_flag );
ULONG MsgWaitForMultipleObjects( int value, HANDLE *h, bool opt1, bool opt2, uint32 input_opt );

// --- other stuff --
HANDLE new_handle(int type);



