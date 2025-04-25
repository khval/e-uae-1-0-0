
#include <exec/types.h>
#include <exec/tasks.h>
#include <exec/libraries.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <proto/exec.h>
#undef __USE_INLINE__
#include <proto/bsdsocket.h>

#include "win32_handle_emu.h"
#include "win32_thread_emu.h"
#include "win32_socket_emu.h"

extern unsigned int primary_signal_mask;

// Helper to convert custom FD_SET to system fd_set
static void convert_fdset_to_native(struct _FD_SET *custom, fd_set *native)
{
	FD_ZERO(native);
	if (!custom) return;
	for (int i = 0; i < custom->fd_count; ++i)
	{
		FD_SET(to_host_sock(custom->fd_array[i]), native);
	}
}

/*
int WSAAsyncSelect(int socket, void *hwnd, unsigned int msg, long event)
{
	struct socketbase *sb = socket_to_sb(to_host_sock(socket));

	if (!sb)
	{
		fprintf(stderr, "WSAAsyncSelect: invalid socket %d\n", socket);
		return -1;
	}

	int index = getsd(sb, socket);
	if (index <= 0 || index > sb->dtablesize)
	{
		printf("WSAAsyncSelect: invalid index %d\n", index);
		return -1;
	}

	sb->mtable[index - 1] = msg;
	sb->ftable[index - 1] = 0;

	if (event & FD_ACCEPT) sb->ftable[index - 1] |= FD_ACCEPT;
	if (event & FD_CONNECT) sb->ftable[index - 1] |= FD_CONNECT;
	if (event & FD_OOB) sb->ftable[index - 1] |= FD_OOB;
	if (event & FD_READ) sb->ftable[index - 1] |= FD_READ;
	if (event & FD_WRITE) sb->ftable[index - 1] |= FD_WRITE;
	if (event & FD_CLOSE) sb->ftable[index - 1] |= FD_CLOSE;
	
	return 0;
}

int win32_select_wrapper(
	long nfds,
	struct _FD_SET *readsocks,
	struct _FD_SET *writesocks,
	struct _FD_SET *exceptsocks,
	struct timeval *timeout)
{
	int resultval = 0;

	fd_set *native_read = NULL, *native_write = NULL, *native_except = NULL;
	HANDLE thread = GetCurrentThread();

	if (!thread || !thread->IS) {
		printf("[error] win32_select_wrapper: no thread or ISocket\n");
		return -1;
	}

	if (readsocks) {
		native_read = (fd_set *)malloc(sizeof(fd_set));
		if (native_read) convert_fdset_to_native(readsocks, native_read);
	}
	if (writesocks) {
		native_write = (fd_set *)malloc(sizeof(fd_set));
		if (native_write) convert_fdset_to_native(writesocks, native_write);
	}
	if (exceptsocks) {
		native_except = (fd_set *)malloc(sizeof(fd_set));
		if (native_except) convert_fdset_to_native(exceptsocks, native_except);
	}

	resultval = thread->IS->WaitSelect(
		nfds,
		native_read,
		native_write,
		native_except,
		timeout,
		NULL
	);

	if (native_read) free(native_read);
	if (native_write) free(native_write);
	if (native_except) free(native_except);

	printf("win32_select_wrapper: finished cleanup, returning %d\n", resultval);

	return resultval;
}
*/

int to_win_sock(int host_sock)
{
	int out =  ((host_sock>-1) ? win_sock_prefix | (host_sock+1) : (host_sock));
	printf("%08lx = %s(%d)\n",out,__FUNCTION__,host_sock);
	return out;
}

int to_host_sock(int win_sock)
{
	int out;

	if ((win_sock & win_sock_prefix_mask) == win_sock_prefix)
	{
		out = win_sock - (win_sock_prefix +1);
	}
	else
	{
		out = win_sock;
	}

	printf("%d = %s(0x%08lx)\n",out,__FUNCTION__,win_sock);

	return out;
}

/*

int win32_WaitSelect(long nfds, struct _FD_SET *readfds, struct _FD_SET *writefds, struct _FD_SET *exceptfds, struct timeval *timeout, void *reserved)
{
	HANDLE handles[FD_SETSIZE];
	ULONG signals[FD_SETSIZE];
	int count = 0;
	ULONG signal_mask = 0;

	struct handle_thread_s *thread = (struct handle_thread_s *)GetCurrentThread();
	if (!thread)
	{
		Printf("win32_WaitSelect(): no current thread!\n");
		return -1;
	}

	// Convert socket FD_SETs to HANDLEs and gather their signal bits
	for (int i = 0; readfds && i < readfds->fd_count && count < FD_SETSIZE; ++i)
	{
		int sock = to_host_sock(readfds->fd_array[i]);
		HANDLE h = socket_to_handle(sock);  // You need to implement this if not already

		if (h && h->type == h_event)
		{
			handles[count] = h;
			signals[count] = h->signal;
			signal_mask |= (1L << h->signal);
			count++;
		}
	}

	if (count == 0 && timeout == NULL) {
		// No valid handles to wait on, and no timeout
		return 0;
	}

	ULONG waited = Wait(signal_mask | (timeout ? (1L << thread->BaseClass.signal_bit) : 0));

	int ready = 0;

	for (int i = 0; i < count; ++i)
	{
		if (waited & (1L << signals[i]))
		{
			// Mark socket as ready
			readfds->fd_array[0] = to_win_sock(handle_to_socket(handles[i])); // Map it back if needed
			readfds->fd_count = 1;
			ready++;
		}
	}

	// Timeout case
	if (timeout && (waited & (1L << thread->BaseClass.signal_bit)))
	{
		Printf("win32_WaitSelect(): timeout expired\n");
		return 0;
	}

	return ready;
}

*/