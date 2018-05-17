#include "sockets.h"
#include "fd.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/syscall.h>  /* __NR_* constants */
#include <linux/net.h>    /* SYS_* constants */
#include <netpacket/packet.h>
#include <klee/klee.h>
#include "klee/Config/config.h"

void klee_warning(const char*);
void klee_warning_once(const char*);
int klee_get_errno(void);


int __socketcall(int type, unsigned long *args) __attribute__((weak));
int __socketcall(int type, unsigned long *args) {
  switch (type) {
  case SYS_SOCKET:
    return __fd_socket(args);
  case SYS_BIND:
    return __fd_bind(args);
  case SYS_CONNECT:
    return __fd_connect(args);
  case SYS_LISTEN:
    return __fd_listen(args);
  case SYS_ACCEPT:
    return __fd_accept(args[0], (struct sockaddr *)args[1], (socklen_t *)args[2]);
  case SYS_GETSOCKNAME:
    return __fd_getsockname(args);
  case SYS_GETPEERNAME:
    return __fd_getpeername(args);
  case SYS_SOCKETPAIR:
    break;
  case SYS_SEND:
    return __fd_send(args[0], (const void *)args[1], (size_t)args[2], args[3]);
  case SYS_RECV:
    return __fd_recv(args[0], (void *)args[1], (size_t)args[2], args[3]);
  case SYS_SENDTO:
    return __fd_sendto(args[0], (const void *)args[1], (size_t)args[2], args[3],
                       (const struct sockaddr *) args[4], (socklen_t) args[5]);
  case SYS_RECVFROM:
    return __fd_recvfrom(args[0], (void *)args[1], (size_t)args[2], args[3],
                         (struct sockaddr *) args[4], (socklen_t *) args[5]);
  case SYS_SHUTDOWN:
    return __fd_shutdown(args);
  case SYS_SETSOCKOPT:
    klee_warning("ignoring setsockopt");
    return 0;
  case SYS_GETSOCKOPT:
    klee_warning("ignoring getsockopt");
    return 0;
  case SYS_SENDMSG:
    return __fd_sendmsg(args[0], (struct msghdr *) args[1], args[2]);
  case SYS_RECVMSG:
    return __fd_recvmsg(args[0], (struct msghdr *) args[1], args[2]);
  }
  printf("__socketcall(type=%d: unknown): errno=ENOSYS\n", type);
  errno = ENOSYS;
  return -1;
}


#define PORT(addr) ( \
    ((struct sockaddr *)(addr))->sa_family == PF_INET  ? ((struct sockaddr_in  *)(addr))->sin_port : \
    ((struct sockaddr *)(addr))->sa_family == PF_INET6 ? ((struct sockaddr_in6 *)(addr))->sin6_port : \
    ((assert(0 && "unsupported domain"), 0)))

/* checks only the port numbers */
#define HAS_ADDR(a) (assert((a)->addr), PORT((a)->addr) != 0)

#define ADDR_COMPATIBLE(addr, addrlen, a) \
    ((addrlen) == (a)->addrlen && (addr)->sa_family == (a)->addr->ss_family)


/* Just to set dfile as a non-NULL value for symbolic datagram sockets */
static exe_disk_file_t dummy_dfile = {0, NULL, NULL, NULL, NULL};


static exe_disk_file_t *__get_sym_stream() {
  //char msg[64];
  //sprintf(msg, "sym streams used/all %d/%d", __exe_fs.n_sym_streams_used, __exe_fs.n_sym_streams);
  //klee_warning(msg);

  if (__exe_fs.n_sym_streams_used >= __exe_fs.n_sym_streams)
    return NULL;
  return &__exe_fs.sym_streams[__exe_fs.n_sym_streams_used++];
}


static exe_disk_file_t *__get_sym_dgram() {
  if (__exe_fs.n_sym_dgrams_used >= __exe_fs.n_sym_dgrams)
    return NULL;
  return &__exe_fs.sym_dgrams[__exe_fs.n_sym_dgrams_used++];
}


static exe_sockaddr_t *__allocate_sockaddr(int domain, exe_sockaddr_t *a) {
  assert(a);
  switch (domain) {
#define SET_SOCKADDR(a, type, family) do { \
    (a)->addr = calloc(1, sizeof(type)); \
    if (!(a)->addr) return NULL; \
    (a)->addr->ss_family = AF_INET; \
    (a)->addrlen = sizeof(type); \
    return (a); \
} while (0)
  case PF_INET:
    SET_SOCKADDR(a, struct sockaddr_in, AF_INET);

  case PF_INET6:
    SET_SOCKADDR(a, struct sockaddr_in6, AF_INET6);

  case PF_PACKET:
    SET_SOCKADDR(a, struct sockaddr_ll, AF_PACKET);
#undef SET_SOCKADDR

  default:
    assert(0 && "unsupported domain");
    return 0;
  }
}

int __fd_socket(unsigned long *args) {

  int domain = args[0];
  int type = args[1];
  int protocol = args[2];

  return socket(domain, type, protocol);
}

int socket(int domain, int type, int protocol) {
  int fd;
  exe_file_t* f;

  if (klee_zest_enabled()) {
    fd = __get_new_fd(&f);
    if (fd < 0) {
      klee_warning("No more file descriptors");
      errno = EMFILE;
      return fd;
    }

    int os_r = syscall(__NR_socket, domain, type, protocol);
    if (os_r < 0) {
      __undo_get_new_fd(f);
      errno = klee_get_errno();
      return os_r;
    } else {
      f->flags |= eSocket;
      f->flags |= eReadable | eWriteable;
      f->fd = os_r;
    }
    return fd;
  }

  switch (domain) {
  case PF_INET:
  case PF_INET6:
  case PF_PACKET:

    switch (type) {
    case SOCK_STREAM:
      /* create a symbolic stream socket */
      fd = __get_new_fd(&f);
      if (fd < 0) return fd;
      f->flags |= eSocket;

      if (!__allocate_sockaddr(domain, &f->local)) {
        __undo_get_new_fd(f);
        errno = ENOMEM;
        return -1;
      }

      f->foreign = malloc(sizeof(*f->foreign));
      if (!f->foreign) {
        free(f->local.addr);
        __undo_get_new_fd(f);
        errno = ENOMEM;
        return -1;
      }

      if (!__allocate_sockaddr(domain, f->foreign)) {
        free(f->foreign);
        free(f->local.addr);
        __undo_get_new_fd(f);
        errno = ENOMEM;
        return -1;
      }

      f->dfile = &dummy_dfile;
      f->flags |= eReadable | eWriteable;
      /* XXX Should check access against mode / stat / possible deletion. */

      f->domain = domain;
      break;

    case SOCK_DGRAM:
    case SOCK_RAW:
    case SOCK_PACKET:
      /* create a symbolic datagram socket */
      fd = __get_new_fd(&f);
      if (fd < 0) return fd;
      f->flags |= eSocket | eDgramSocket;

      if (!__allocate_sockaddr(domain, &f->local)) {
        __undo_get_new_fd(f);
        errno = ENOMEM;
        return -1;
      }

      f->foreign = malloc(sizeof(*f->foreign));
      if (!f->foreign) {
        free(f->local.addr);
        __undo_get_new_fd(f);
        errno = ENOMEM;
        return -1;
      }

      if (!__allocate_sockaddr(domain, f->foreign)) {
        free(f->foreign);
        free(f->local.addr);
        __undo_get_new_fd(f);
        errno = ENOMEM;
        return -1;
      }

      f->dfile = &dummy_dfile;
      f->flags |= eReadable | eWriteable;
      /* XXX Should check access against mode / stat / possible deletion. */
      break;

    default:
      klee_warning("unsupported socket type");
      errno = ESOCKTNOSUPPORT;
      return -1;
    } /* switch (type) */
    break;

  default:
    /* TODO: could easily be extended to delegate to the native socket call */
    klee_warning("unsupported socket protocol");
    errno = EPROTONOSUPPORT;
    return -1;
  } /* switch (domain) */

  return fd;
}

int __fd_bind(unsigned long *args) {
  int sockfd = args[0];
  const struct sockaddr *addr = (const struct sockaddr *) args[1];
  socklen_t addrlen = args[2];

  return bind(sockfd, addr, addrlen);
}

int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
  int os_r;
  exe_file_t *f = __get_file(sockfd);

  if (!f) {
    errno = EBADF;      /* Bad file number */
    return -1;
  }

  if (!(f->flags & eSocket)) {
    errno = ENOTSOCK;   /* Socket operation on non-socket */
    return -1;
  }

  if (f->dfile) {
    /* symbolic file descriptor */
    assert(f->local.addr);
    if (!(f->flags & eDgramSocket) && HAS_ADDR(&f->local)) {
      errno = EINVAL;   /* The socket is already bound to an address */
      return -1;
    }
    if (!ADDR_COMPATIBLE(addr, addrlen, &f->local)) {
      errno = EINVAL;   /* The addrlen is wrong */
      return -1;
    }
    /* assign the address to the socket */
    memcpy(f->local.addr, addr, addrlen);
    if (PORT(f->local.addr) == 0) {
      /* TODO: choose an ephemeral port */
    }
  }

  else {
    /* concrete file descriptor */

    os_r = syscall(__NR_bind, f->fd, addr, addrlen);
    if (os_r < 0) {
      errno = klee_get_errno();
    }
    return os_r;
  }

  return 0;
}

int __fd_connect(unsigned long *args) {
  int sockfd = args[0];
  const struct sockaddr *addr = (const struct sockaddr *) args[1];
  socklen_t addrlen = args[2];
  return connect(sockfd, addr, addrlen);
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
  int os_r;
  exe_file_t *f = __get_file(sockfd);

  if (!f) {
    errno = EBADF;      /* Bad file number */
    return -1;
  }

  if (!(f->flags & eSocket)) {
    errno = ENOTSOCK;   /* Socket operation on non-socket */
    return -1;
  }

  if (f->dfile) {
    /* symbolic file descriptor */

    assert(f->foreign->addr);
//    if (!(f->flags & eDgramSocket)) {
//      assert(HAS_ADDR(f->foreign));
//      errno = EISCONN;  /* Transport endpoint is already connected */
//      return -1;
//    }
    if (!ADDR_COMPATIBLE(addr, addrlen, f->foreign)) {
      errno = EINVAL;   /* The addrlen is wrong */
      return -1;
    }
    if (PORT(addr) == 0) {
      errno = EADDRNOTAVAIL;  /* Cannot assign requested address */
    }
    if (PORT(f->local.addr) == 0) {
      /* TODO: choose an ephemeral port */
    }
    /* assign the address to the socket */
    memcpy(f->foreign->addr, addr, addrlen);
  }

  else {
    /* concrete file descriptor */
    assert(0 && "disabled concrete, reable!");
    os_r = syscall(__NR_connect, f->fd, addr, addrlen);
    if (os_r < 0) {
      errno = klee_get_errno();
    }
    return os_r;
  }
  return 0;
}


int __fd_listen(unsigned long *args) {
  int sockfd = args[0];
  int backlog = args[1];

  return listen(sockfd, backlog);
}

int listen(int sockfd, int backlog) {
  int os_r;
  exe_file_t *f = __get_file(sockfd);

  if (!f) {
    errno = EBADF;      /* Bad file number */
    return -1;
  }

  if (!(f->flags & eSocket)) {
    errno = ENOTSOCK;   /* Socket operation on non-socket */
    return -1;
  }

  if (f->flags & eDgramSocket) {
    errno = EOPNOTSUPP; /* Operation not supported on transport endpoint */
    return -1;
  }

  if (f->dfile) {
    /* assume success for symbolic socket */
    os_r = 0;
  } else {
    os_r = syscall(__NR_listen, f->fd, backlog);
    if (os_r < 0) {
      errno = klee_get_errno();
    }
  }

  f->flags |= eListening;

  return os_r;
}

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
  return __fd_accept(sockfd, addr, addrlen);
}

int __fd_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
  int connfd;
  exe_file_t *connf;
  exe_file_t *f = __get_file(sockfd);

  if (!f) {
    errno = EBADF;      /* Bad file number */
    return -1;
  }

  if (!(f->flags & eSocket)) {
    errno = ENOTSOCK;   /* Socket operation on non-socket */
    return -1;
  }

  if (f->flags & eDgramSocket) {
    errno = EOPNOTSUPP; /* Operation not supported on transport endpoint */
    return -1;
  }

  if (!(f->flags & eListening)) {
    errno = EINVAL;     /* socket is not listening for connections */
    return -1;
  }

  if (0 /* queue empty */) {
    if (0 /* non-blocking socket */) {
      errno = EAGAIN;
      return -1;
    }
    /* wait for a pending connection */
  }

  if (f->dfile) {
    /* (pretend to) get a pending connection */
    connfd = __get_new_fd(&connf);
    if (connfd < 0) return connfd;
    connf->flags |= eSocket;
  
    connf->dfile = __get_sym_stream();
    if (!connf->dfile) {
      klee_warning("No more symbolic streams");
      __undo_get_new_fd(connf);
      errno = ENFILE;
      return -1;
    }
  
    /* Allocate the local address */
    if (!__allocate_sockaddr(f->domain, &connf->local)) {
      --__exe_fs.n_sym_streams_used;  /* undo __get_sym_stream() */
      __undo_get_new_fd(connf);
      errno = ENOMEM;
      return -1;
    }
    /* TODO: Fill the local address */
  
    /* Adjust the source address to the local address */
    connf->dfile->src->addrlen         = connf->local.addrlen;
    connf->dfile->src->addr->ss_family = connf->local.addr->ss_family;
  
    /* For TCP, foreign simply points to dfile->src */
    connf->foreign = connf->dfile->src;
  
    /* Fill the peer socket address */
    if (addr) {
      klee_check_memory_access(addr, *addrlen);
      if (*addrlen < connf->foreign->addrlen) {
        free(connf->foreign->addr);
        free(connf->local.addr);
        --__exe_fs.n_sym_streams_used;  /* undo __get_sym_stream() */
        __undo_get_new_fd(connf);
        errno = EINVAL;
        return -1;
      }
      memcpy(addr, connf->foreign->addr, connf->foreign->addrlen);
      *addrlen = connf->foreign->addrlen; /* man page: when addr is NULL, nothing is filled in */
    }  
  
    connf->flags |= eReadable | eWriteable;
    /* XXX Should check access against mode / stat / possible deletion. */
    //char rmsg[64];
    //sprintf(rmsg, "accept returning %d", connfd);
    //klee_warning(rmsg);
    return connfd;
  } else {
    exe_file_t* fchild;
    int fd = __get_new_fd(&fchild);
    if (fd < 0) {
      errno = ENOMEM;
      return -1;
    }
    int os_r = syscall(__NR_accept, f->fd, addr, addrlen);
    if (os_r < 0) {
      errno = klee_get_errno();
      __undo_get_new_fd(fchild);
      return os_r;
    }
    fchild->flags |= eSocket;
    fchild->flags |= eReadable | eWriteable;
    fchild->fd = os_r;
    return fd;
  }
}


int __fd_getsockname(unsigned long *args)
{
  int sockfd = args[0];
  struct sockaddr *addr = (struct sockaddr *) args[1];
  socklen_t *addrlen = (socklen_t *) args[2];

  return getsockname(sockfd, addr, addrlen);
}

int getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
  exe_file_t *f = __get_file(sockfd);
  int os_r;

  if (!f) {
    errno = EBADF;      /* Bad file number */
    return -1;
  }

  if (!(f->flags & eSocket)) {
    errno = ENOTSOCK;   /* Socket operation on non-socket */
    return -1;
  }

  if (f->dfile) {
    /* symbolic file descriptor */
    assert(f->local.addr);
    if (*addrlen < f->local.addrlen) {
      errno = EINVAL;
      return -1;
    }
    memcpy(addr, f->local.addr, f->local.addrlen);
    *addrlen = f->local.addrlen;
  } else {
    /* concrete file descriptor */
    os_r = syscall(__NR_getsockname, f->fd, addr, addrlen);
    if (os_r < 0) {
      errno = klee_get_errno();
      return -1;
    }
  }

  return 0;
}


int __fd_getpeername(unsigned long *args)
{
  int sockfd = args[0];
  struct sockaddr *addr = (struct sockaddr *) args[1];
  socklen_t *addrlen = (socklen_t *) args[2];
  return getpeername(sockfd, addr, addrlen);
}

int getpeername(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
  int os_r;
  exe_file_t *f = __get_file(sockfd);

  if (!f) {
    errno = EBADF;      /* Bad file number */
    return -1;
  }

  if (!(f->flags & eSocket)) {
    errno = ENOTSOCK;   /* Socket operation on non-socket */
    return -1;
  }

  if (f->dfile) {
    /* symbolic file descriptor */
    assert(f->foreign->addr);
    if (*addrlen < f->foreign->addrlen) {
      errno = EINVAL;
      return -1;
    }
    memcpy(addr, f->foreign->addr, f->foreign->addrlen);
    *addrlen = f->foreign->addrlen;
  }

  else {
    /* concrete file descriptor */
    os_r = syscall(__NR_getpeername, f->fd, addr, addrlen);;
    if (os_r < 0) {
      errno = klee_get_errno();
      return -1;
    }
  }

  return 0;
}


int __fd_shutdown(unsigned long *args) {
  int sockfd = args[0];
  int how = args[1];
  return shutdown(sockfd, how);
}

int shutdown(int sockfd, int how) {
  exe_file_t *f = __get_file(sockfd);

  if (!f) {
    errno = EBADF;      /* Bad file number */
    return -1;
  }

  if (!(f->flags & eSocket)) {
    errno = ENOTSOCK;   /* Socket operation on non-socket */
    return -1;
  }

  if (f->dfile) {
    /* symbolic file descriptor */

    switch (how) {
    case SHUT_RD:
      f->flags &= ~eReadable; break;
    case SHUT_WR:
      f->flags &= ~eWriteable; break;
    case SHUT_RDWR:
      f->flags &= ~(eReadable | eWriteable); break;
    default:
      errno = EINVAL;
      return -1;
    }
  }
  else {
    /* concrete file descriptor */
    int os_r;

    os_r = syscall(__NR_shutdown, sockfd, how);
    if (os_r == -1) {
      errno = klee_get_errno();
      return -1;
    }
  }

  return 0;
}


ssize_t send(int fd, const void *buf, size_t len, int flags) {
  return __fd_send(fd, buf, len, flags);
}

ssize_t __fd_send(int fd, const void *buf, size_t len, int flags)
{
  return __fd_sendto(fd, buf, len, flags, NULL, 0);
}

ssize_t recv(int fd, void *buf, size_t len, int flags) {
  return __fd_recv(fd, buf, len, flags);
}

ssize_t __fd_recv(int fd, void *buf, size_t len, int flags)
{
  return __fd_recvfrom(fd, buf, len, flags, NULL, NULL);
}

ssize_t sendto(int fd, const void *buf, size_t len, int flags, const struct sockaddr *to, socklen_t tolen)
{
  return __fd_sendto(fd, buf, len, flags, to, tolen);
}

ssize_t __fd_sendto(int fd, const void *buf, size_t len, int flags, const struct sockaddr *to, socklen_t tolen)
{
  struct iovec iov;
  struct msghdr msg;

  iov.iov_base = (void *) buf;            /* const_cast */
  iov.iov_len = len;

  msg.msg_name = (struct sockaddr *) to;  /* const_cast */
  msg.msg_namelen = tolen;
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_control = NULL;
  msg.msg_controllen = 0;
  msg.msg_flags = flags;

  return __fd_sendmsg(fd, &msg, flags);
}

ssize_t recvfrom(int fd, void *buf, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen)
{
  return __fd_recvfrom(fd, buf, len, flags, from, fromlen);
}

ssize_t __fd_recvfrom(int fd, void *buf, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen)
{
  struct iovec iov;
  struct msghdr msg;
  ssize_t s;

  //if (from != NULL && fromlen == NULL) {
  //  errno = EFAULT;
  //  return -1;
  //}

  iov.iov_base = buf;
  iov.iov_len = len;

  msg.msg_name = from;
  msg.msg_namelen = fromlen ? *fromlen : /* In this case, from should be NULL */ 0;
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_control = NULL;
  msg.msg_controllen = 0;
  msg.msg_flags = flags;

  s = __fd_recvmsg(fd, &msg, flags);

  if (fromlen) *fromlen = msg.msg_namelen;
  return s;
}


ssize_t sendmsg(int fd, const struct msghdr *msg, int flags)
{
  return __fd_sendmsg(fd, msg, flags);
}

ssize_t __fd_sendmsg(int fd, const struct msghdr *msg, int flags)
{
  exe_file_t *f = __get_file(fd);
  if (!f) {
    errno = EBADF;      /* Bad file number */
    return -1;
  }

  if (!(f->flags & eSocket)) {
    errno = ENOTSOCK;   /* Socket operation on non-socket */
    return -1;
  }

  if (msg == NULL) {
    errno = EFAULT;     /* Bad address */
    return -1;
  }

  if (!f->dfile) {
    /* concrete socket */
    assert(0 && "not supported yet");
  } else {
    /* symbolic socket */
    if (!(f->flags & eDgramSocket)) {
      assert(f->foreign->addr);
      if (!HAS_ADDR(f->foreign)) {
        errno = ENOTCONN; /* Transport endpoint is not connected */
        return -1;
      }
    }
    else {
      if (!HAS_ADDR(f->foreign) && msg->msg_name == NULL) {
        errno = ENOTCONN; /* Transport endpoint is not connected */
        return -1;
      }

      if (HAS_ADDR(f->foreign) && msg->msg_name != NULL) {
        errno = EISCONN;  /* Transport endpoint is already connected */
        return -1;
      }
    }

    if (msg->msg_name != NULL) {
      klee_check_memory_access(msg->msg_name, msg->msg_namelen);
      /* ignore the destination */
    }

    if (flags != 0)
      klee_warning("flags is not zero, ignoring");

    return __fd_gather_write(f, msg->msg_iov, msg->msg_iovlen);
  }
}

ssize_t recvmsg(int fd, struct msghdr *msg, int flags)
{
  return __fd_recvmsg(fd, msg, flags);
}

ssize_t __fd_recvmsg(int fd, struct msghdr *msg, int flags)
{
  exe_file_t *f = __get_file(fd);
  if (!f) {
    errno = EBADF;      /* Bad file number */
    return -1;
  }

  if (!(f->flags & eSocket)) {
    errno = ENOTSOCK;   /* Socket operation on non-socket */
    return -1;
  }

  if (msg == NULL) {
    errno = EFAULT;     /* Bad address */
    return -1;
  }

  if (!f->dfile) {
    /* concrete socket */
    assert(0 && "not supported yet");
  } else {
    /* symbolic socket */
    if (__fd_attach_dgram(f) < 0)
      return -1;

    if (msg->msg_name != NULL) {
      klee_check_memory_access(msg->msg_name, msg->msg_namelen);
      memcpy(msg->msg_name, f->dfile->src->addr, f->dfile->src->addrlen);
    }
    msg->msg_namelen = f->dfile->src->addrlen;

    if (flags != 0)
      klee_warning("flags is not zero, ignoring");

    return __fd_scatter_read(f, msg->msg_iov, msg->msg_iovlen);
  }
}

ssize_t __fd_attach_dgram(struct exe_file_t *f)
{
  if (!(f->flags & eDgramSocket)) {
    if (!HAS_ADDR(f->foreign)) {
      errno = ENOTCONN; /* Transport endpoint is not connected */
      return -1;
    }
  }
  else {
    /* assign a new symbolic datagram source for datagram sockets */
    f->off = 0;
    f->dfile = __get_sym_dgram();
    if (!f->dfile) {
      /* no more datagrams; won't happen in the real world */
      f->dfile = &dummy_dfile;
      errno = ECONNREFUSED;
      return -1;
    }
  }
  return 0;
}

int getsockopt(int sockfd, int level, int optname,
                                     void *optval, socklen_t *optlen)
{
  return 0;
}

int setsockopt(int sockfd, int level, int optname,
                                            const void *optval, socklen_t optlen)
{
  return 0;
}
