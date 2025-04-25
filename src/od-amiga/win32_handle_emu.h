
enum
{
	h_signal,
	h_event,
	h_thread
};

struct handle_s
{
	USHORT type;
	APTR object;
	uint32 index;
};

struct handle_thread_s
{
	struct handle_s BaseClass;
	struct Library *SocketBase;
	struct SocketIFace *IS;
	void (*func) ( struct handle_thread_s *thread );
	struct MsgPort *timerPort;
	struct timerequest *timerIO;
	ULONG timer;
};

struct handle_event_s
{
	struct handle_s BaseClass;
	char *name;
	struct List waiting;
	APTR mux;
};

typedef struct handle_s *HANDLE;

HANDLE new_handle(int type);

void CloseHandle( HANDLE h );

