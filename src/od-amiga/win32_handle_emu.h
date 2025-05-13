
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

struct handle_ptr
{
	struct handle_s *ptr;
	BOOL lock;
};

struct handle_thread_s;

struct thread_s
{
	struct Library *SocketBase;
	struct SocketIFace *IS;
	void (*func) ( struct handle_thread_s *thread );
	struct MsgPort *ProxyPort;
	struct MsgPort *timerPort;
	struct timerequest *timerIO;
	BOOL timer_used;
	ULONG timerOpenError;
	BPTR  output ;
    	BOOL running;	
	BOOL lock;
};

struct handle_thread_s
{
	struct handle_s base;
	struct thread_s t;
};

struct handle_event_s
{
	struct handle_s BaseClass;
	char *name;
	struct List waiting;
	APTR mux;
};

typedef struct handle_ptr HANDLE;

HANDLE new_handle(int type);

void CloseHandle( HANDLE *h_ptr );

