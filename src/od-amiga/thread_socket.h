
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

struct emu_sock
{
	struct MsgPort *ProxyPort;
	int host_socket;
};

struct ThreadMessage
{
	struct Message base;
	void (*callback_fn) ( TrapContext *context);
	TrapContext *context; //	DO-D7 / A0-A7
	struct Task *sender;
};

void send_thread_msg( struct socketbase *sb, TrapContext *context, void (*msg_callback_fn) (TrapContext *context) );


#define MaxSockets 1024

struct emu_sock sockets[MaxSockets];
struct ThreadMessage ThreadMessages[MaxSockets];


#define to_host_sock(emu_sock) sockets[emu_sock].host_socket

extern int new_guest_socket( void );
extern void free_guest_socket( int n );
extern int to_guest_sock( struct MsgPort *host_port, int host_socket); 

#define MAXGETHOSTSTRUCT 1024

// Standard wrappers
#define thread_accept(sock, paddr, psize) \
    ((struct handle_thread_s *) thread)->t.IS->accept(to_host_sock(sock), paddr, psize)

#define thread_connect(sock, paddr, size) \
    ((struct handle_thread_s *) thread)->t.IS->connect(to_host_sock(sock), paddr, size)

#define thread_bind(sock, addr, size) \
    ((struct handle_thread_s *) thread)->t.IS->bind(to_host_sock(sock), addr, size)

#define thread_listen(sock, backlog) \
    ((struct handle_thread_s *) thread)->t.IS->listen(to_host_sock(sock), backlog)

#define thread_send(sock, buf, len, flags) \
    ((struct handle_thread_s *) thread)->t.IS->send(to_host_sock(sock), buf, len, flags)

#define thread_recv(sock, buf, len, flags) \
    ((struct handle_thread_s *) thread)->t.IS->recv(to_host_sock(sock), buf, len, flags)

#define thread_sendto(sock, buf, len, flags, to, tolen) \
    ((struct handle_thread_s *) thread)->t.IS->sendto(to_host_sock(sock), buf, len, flags, to, tolen)

#define thread_recvfrom(sock, buf, len, flags, from, fromlen) \
    ((struct handle_thread_s *) thread)->t.IS->recvfrom(to_host_sock(sock), buf, len, flags, from, fromlen)

#define thread_shutdown(sock, how) \
    ((struct handle_thread_s *) thread)->t.IS->shutdown(to_host_sock(sock), how)

#define thread_CloseSocket(host_socket) \
    ((struct handle_thread_s *) thread)->t.IS->CloseSocket(host_socket)

#define thread_setsockopt(sock, level, optname, optval, optlen) \
    ((struct handle_thread_s *) thread)->t.IS->setsockopt(to_host_sock(sock), level, optname, optval, optlen)

#define thread_getsockopt(sock, level, optname, optval, optlen) \
    ((struct handle_thread_s *) thread)->t.IS->getsockopt(to_host_sock(sock), level, optname, optval, optlen)

#define thread_getsockname(sock, name, namelen) \
    ((struct handle_thread_s *) thread)->t.IS->getsockname(to_host_sock(sock), name, namelen)

#define thread_IoctlSocket(sock, cmd, argp) \
    ((struct handle_thread_s *) thread)->t.IS->IoctlSocket(to_host_sock(sock), cmd, argp)

#define thread_WaitSelect(nfds, readfds, writefds, exceptfds, timeout, signals) \
    ((struct handle_thread_s *) thread)->t.IS->WaitSelect(nfds, readfds, writefds, exceptfds, timeout, signals)

#define thread_gethostbyname(name) \
    ((struct handle_thread_s *) thread)->t.IS->gethostbyname(name)

#define thread_gethostbyaddr(addr,len,type) \
    ((struct handle_thread_s *) thread)->t.IS->gethostbyaddr(addr,len,type)

#define thread_inet_network(cp) \
    ((struct handle_thread_s *) thread)->t.IS->inet_network(cp)

#define thread_gethostbyname(name) \
    ((struct handle_thread_s *) thread)->t.IS->gethostbyname(name)

#define thread_inet_MakeAddr(net,lna) \
    ((struct handle_thread_s *) thread)->t.IS->Inet_MakeAddr(net,lna)

#define thread_Inet_addr(cp) \
    ((struct handle_thread_s *) thread)->t.IS->inet_addr(cp)

#define thread_Inet_NtoA(in) \
    ((struct handle_thread_s *) thread)->t.IS->Inet_NtoA(in)

#define thread_getprotobynumber(proto) \
    ((struct handle_thread_s *) thread)->t.IS->getprotobynumber(proto)

#define thread_getservbyname(name,proto) \
    ((struct handle_thread_s *) thread)->t.IS->getservbyname(name,proto)

#define thread_getservbyport(port,proto) \
    ((struct handle_thread_s *) thread)->t.IS->getservbyport((port),(proto))

#define thread_gethostname(name,namelen) \
    ((struct handle_thread_s *) thread)->t.IS->gethostname((name),(namelen))

#define thread_Dup2Socket(old_socket,new_socket) \
    ((struct handle_thread_s *) thread)->t.IS->Dup2Socket((old_socket),(new_socket))

#define thread_getprotobyname(name) \
    ((struct handle_thread_s *) thread)->t.IS->getprotobyname(name)

#define thread_SetErrnoPtr(ptr,size) \
    ((struct handle_thread_s *) thread)->t.IS->SetErrnoPtr(ptr,size)

#define thread_SocketBaseTagList(tag_list) \
    ((struct handle_thread_s *) thread)->t.IS->SocketBaseTagList(tag_list)

#define thread_Errno() \
    ((struct handle_thread_s *) thread)->t.IS->Errno()

#define thread_Inet_LnaOf(in) \
    ((struct handle_thread_s *) thread)->t.IS->Inet_LnaOf(in)

#define thread_Inet_NetOf(in) \
    ((struct handle_thread_s *) thread)->t.IS->Inet_NetOf(in)

// Socket creation
#define thread_socket(af, type, protocol) \
   ((struct handle_thread_s *) thread)->t.IS->socket(af, type, protocol) 

#define thread_ObtainSocket(id,domain,type,protocol ) \
   ((struct handle_thread_s *) thread)->t.IS->ObtainSocket(id,domain,type,protocol) 

#define thread_ReleaseSocket(fd, id) \
   ((struct handle_thread_s *) thread)->t.IS->ReleaseSocket(fd, id) 

