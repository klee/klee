// RUN: %llvmgcc %s -emit-llvm -O0 -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --posix-runtime %t1.bc --sym-datagrams 1 10 --sym-streams 1 10 > %t.log
// RUN: grep -q Success %t.log
// UN: gcc -g %s -o SocketSelect
// UN: %runbout ./SocketSelect %t.klee-out/test000001.bout > %t2.log
// UN: grep -q Success %t2.log

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
//#include "select-noasm.h"

#undef FD_SET
#undef FD_CLR
#undef FD_ISSET
#undef FD_ZERO
#define FD_SET(n, p) (__FDS_BITS(p)[(n) / NFDBITS] |= (1 << ((n) % NFDBITS)))
#define FD_CLR(n, p) (__FDS_BITS(p)[(n) / NFDBITS] &= ~(1 << ((n) % NFDBITS)))
#define FD_ISSET(n, p) (__FDS_BITS(p)[(n) / NFDBITS] & (1 << ((n) % NFDBITS)))
#define FD_ZERO(p) memset((char *)(p), '\0', sizeof(*(p)))

int make_datagram_socket() {
  struct sockaddr_in server_addr;
  int fd, r;

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(547); // dhcpv6-server

  fd = socket(PF_INET, SOCK_DGRAM, 0);
  if (fd < 0) {
    perror("socket");
    return fd;
  }

  r = bind(fd, (const struct sockaddr *)&server_addr, sizeof(server_addr));
  if (r < 0) {
    perror("bind");
    return r;
  }

  return fd;
}

int make_listening_stream_socket() {
  struct sockaddr_in server_addr;
  int fd, r;

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(80); // www

  fd = socket(PF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    perror("socket");
    return fd;
  }

  r = bind(fd, (const struct sockaddr *)&server_addr, sizeof(server_addr));
  if (r < 0) {
    perror("bind");
    return r;
  }

  r = listen(fd, 5);
  if (r < 0) {
    perror("listen");
    return r;
  }

  return fd;
}

int main(int argc, char **argv) {
  int fd[3], max_fd = -1, r;
  fd_set in_set;
  unsigned result = 0;

  printf("making a datagram socket into fd[0]\n");
  fd[0] = make_datagram_socket();
  if (fd[0] < 0)
    return EXIT_FAILURE;

  printf("making a listening stream socket into fd[1]\n");
  fd[1] = make_listening_stream_socket();
  if (fd[1] < 0)
    return EXIT_FAILURE;

  fd[2] = -1;

  while (fd[0] >= 0 || fd[1] >= 0 || fd[2] >= 0) {
    struct sockaddr_in client_addr;
    socklen_t client_addrlen;
    char buf[100];
    unsigned flags = 0;

    FD_ZERO(&in_set);
    if (fd[0] >= 0) {
      FD_SET(fd[0], &in_set);
      if (fd[0] > max_fd)
        max_fd = fd[0];
    }
    if (fd[1] >= 0) {
      FD_SET(fd[1], &in_set);
      if (fd[1] > max_fd)
        max_fd = fd[1];
    }
    if (fd[2] >= 0) {
      FD_SET(fd[2], &in_set);
      if (fd[2] > max_fd)
        max_fd = fd[2];
    }

    r = select(max_fd + 1, &in_set, NULL, NULL, NULL);
    if (r < 0) {
      perror("select");
      break;
    }
    if (r == 0)
      break;

    /* test these first because fds could change */
    if (fd[0] >= 0 && FD_ISSET(fd[0], &in_set))
      flags |= 01;
    if (fd[1] >= 0 && FD_ISSET(fd[1], &in_set))
      flags |= 02;
    if (fd[2] >= 0 && FD_ISSET(fd[2], &in_set))
      flags |= 04;

    if (flags & 01) {
      client_addrlen = sizeof(client_addr);
      r = recvfrom(fd[0], buf, sizeof buf, 0, (struct sockaddr *)&client_addr,
                   &client_addrlen);
      if (r < 0)
        perror("recvfrom");
      else {
        printf("received %u bytes through fd[0]\n", r);
        if (r == 10)
          result |= 01;
      }
      if (r <= 0) {
        close(fd[0]);
        fd[0] = -1;
      }
    }

    if (flags & 02) {
      client_addrlen = sizeof(client_addr);
      fd[2] = accept(fd[1], (struct sockaddr *)&client_addr, &client_addrlen);
      if (fd[2] < 0)
        perror("accept");
      else {
        printf("accepting connection through fd[1] into fd[2]\n");
        result |= 02;
      }
      if (fd[2] < 0) {
        close(fd[1]);
        fd[1] = -1;
      }
    }

    if (flags & 04) {
      r = recv(fd[2], buf, sizeof buf, 0);
      if (r < 0)
        perror("recv");
      else {
        printf("received %u bytes through fd[2]\n", r);
        if (r == 10)
          result |= 04;
      }
      if (r <= 0) {
        close(fd[2]);
        fd[2] = -1;
      }
    }
  }

  if (fd[2] >= 0)
    close(fd[2]);
  if (fd[1] >= 0)
    close(fd[1]);
  if (fd[0] >= 0)
    close(fd[0]);

  if ((result & 07) == 07) {
    printf("Success\n");
    return EXIT_SUCCESS;
  } else
    return EXIT_FAILURE;
}
