
void thread_start_func( void );
void thread_final_func( void );

struct handle_thread_s *GetCurrentThread( void );
struct handle_thread_s *new_thread( APTR func, int index );

BOOL is_in_hThreads( struct Task *task );

extern int find_new_thread_id(void);

// expect to never use more then one!!
#define MaxThreadMessages	8

#define MAX_SELECT_THREADS 64
extern HANDLE hThreads[MAX_SELECT_THREADS];

void socket_proxy_thread(struct handle_thread_s *thread);



