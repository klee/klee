#ifndef __EXE_SOCKETS__
#define __EXE_SOCKETS__

#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

struct exe_file_t;

int socket(int domain, int type, int protocol) __attribute__((weak));;
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int listen(int sockfd, int backlog);
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int shutdown(int sockfd, int how);
int getsockname(int sockfd,  struct sockaddr *addr, socklen_t *addrlen);
int getpeername(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
ssize_t send(int fd, const void *buf, size_t len, int flags);
ssize_t recv(int fd, void *buf, size_t len, int flags);
ssize_t sendto(int fd, const void *buf, size_t len, int flags, const struct sockaddr *to, socklen_t tolen);
ssize_t recvfrom(int fd, void *buf, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen);
ssize_t sendmsg(int fd, const struct msghdr *msg, int flags);
ssize_t recvmsg(int fd, struct msghdr *msg, int flags);
int getsockopt(int sockfd, int level, int optname,
                                            void *optval, socklen_t *optlen);
int setsockopt(int sockfd, int level, int optname,
                                            const void *optval, socklen_t optlen);


int __fd_socket(unsigned long *args);
int __fd_bind(unsigned long *args);
int __fd_connect(unsigned long *args);
int __fd_listen(unsigned long *args);
int __fd_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int __fd_shutdown(unsigned long *args);
int __fd_getsockname(unsigned long *args);
int __fd_getpeername(unsigned long *args);
ssize_t __fd_send(int fd, const void *buf, size_t len, int flags);
ssize_t __fd_recv(int fd, void *buf, size_t len, int flags);
ssize_t __fd_sendto(int fd, const void *buf, size_t len, int flags, const struct sockaddr *to, socklen_t tolen);
ssize_t __fd_recvfrom(int fd, void *buf, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen);
ssize_t __fd_sendmsg(int fd, const struct msghdr *msg, int flags);
ssize_t __fd_recvmsg(int fd, struct msghdr *msg, int flags);
ssize_t __fd_attach_dgram(struct exe_file_t *f);

#endif
