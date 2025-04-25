


void thread_start_func( void );
void thread_final_func( void );

HANDLE GetCurrentThread( void );
HANDLE new_thread( APTR func, int index );
void SetThreadPriority(HANDLE thread, int pri);
BOOL is_in_hThreads( struct Task *task );
int GetHThread( struct Task *task );
void trigger_thread_event( uint32 bit );

#define MAX_SELECT_THREADS 64
#define MAIN_THREAD_INDEX (MAX_SELECT_THREADS-1)

extern HANDLE hThreads[MAX_SELECT_THREADS];

extern uint64 thread_signal_mask;

