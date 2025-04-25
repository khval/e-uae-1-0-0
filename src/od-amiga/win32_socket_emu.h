
struct _FD_SET
{
	union {
		unsigned long fds_bits[howmany(FD_SETSIZE, NFDBITS)];
		long fd_array[howmany(FD_SETSIZE, NFDBITS)];
	};

	int fd_count;
} _FD_SET;

#define win_sock_prefix 0xCC000000
#define win_sock_prefix_mask 0xFF000000

int to_win_sock(int host_sock); 
int to_host_sock(int win_sock); 

#define MAXGETHOSTSTRUCT 1024

// Standard wrappers
#define win32_accept(sock, paddr, psize) \
    ((struct handle_thread_s *) thread)->IS->accept(to_host_sock(sock), paddr, psize)

#define win32_connect(sock, paddr, size) \
    ((struct handle_thread_s *) thread)->IS->connect(to_host_sock(sock), paddr, size)

#define win32_bind(sock, addr, size) \
    ((struct handle_thread_s *) thread)->IS->bind(to_host_sock(sock), addr, size)

#define win32_listen(sock, backlog) \
    ((struct handle_thread_s *) thread)->IS->listen(to_host_sock(sock), backlog)

#define win32_send(sock, buf, len, flags) \
    ((struct handle_thread_s *) thread)->IS->send(to_host_sock(sock), buf, len, flags)

#define win32_recv(sock, buf, len, flags) \
    ((struct handle_thread_s *) thread)->IS->recv(to_host_sock(sock), buf, len, flags)

#define win32_sendto(sock, buf, len, flags, to, tolen) \
    ((struct handle_thread_s *) thread)->IS->sendto(to_host_sock(sock), buf, len, flags, to, tolen)

#define win32_recvfrom(sock, buf, len, flags, from, fromlen) \
    ((struct handle_thread_s *) thread)->IS->recvfrom(to_host_sock(sock), buf, len, flags, from, fromlen)

#define win32_shutdown(sock, how) \
    ((struct handle_thread_s *) thread)->IS->shutdown(to_host_sock(sock), how)

#define win32_closesocket(sock) \
    ((struct handle_thread_s *) thread)->IS->CloseSocket(to_host_sock(sock))

#define win32_setsockopt(sock, level, optname, optval, optlen) \
    ((struct handle_thread_s *) thread)->IS->setsockopt(to_host_sock(sock), level, optname, optval, optlen)

#define win32_getsockopt(sock, level, optname, optval, optlen) \
    ((struct handle_thread_s *) thread)->IS->getsockopt(to_host_sock(sock), level, optname, optval, optlen)

#define win32_IoctlSocket(sock, cmd, argp) \
    ((struct handle_thread_s *) thread)->IS->IoctlSocket(to_host_sock(sock), cmd, argp)

#define native_WaitSelect(nfds, readfds, writefds, exceptfds, timeout, signals) \
    ((struct handle_thread_s *) thread)->IS->WaitSelect(nfds, readfds, writefds, exceptfds, timeout, signals)

// Socket creation
#define win32_socket(af, type, protocol) \
    to_win_sock( ((struct handle_thread_s *) thread)->IS->socket(af, type, protocol) )


