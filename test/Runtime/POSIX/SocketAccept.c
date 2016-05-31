// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --posix-runtime %t1.bc --sym-streams 1 10 > %t.log
// RUN: grep -q Success %t.log

/* Testing that accept succeeds if performed on a listening socket */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

int main(int argc, char **argv) {
  struct sockaddr_in server_addr, client_addr;
  int client_len;

  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  int conn_fd;
  // fprintf(stderr, "socket_fd = %d\n", socket_fd);

  if (socket_fd < 0) {
    perror("socket() returned error");
    exit(1);
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(1080);

  int r = bind(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
  if (r < 0) {
    perror("bind() returned error");
    exit(1);
  }

  r = listen(socket_fd, 1);
  if (r < 0) {
    perror("listen() returned error");
    exit(1);
  }

  client_len = sizeof(client_addr);
  conn_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &client_len);
  if (conn_fd < 0) {
    perror("accept() returned error");
    exit(1);
  }

#if 0
  switch (client_addr.sin_family) {
    char buf[INET_ADDRSTRLEN];
  case AF_INET:
    if (inet_ntop(AF_INET, &client_addr.sin_addr, buf, sizeof buf) == NULL)
      perror("inet_ntop() returned error");
    else {
    /*printf("remote address: AF_INET\n");*/
      printf("remote address: AF_INET ");
      printf("%08X ", ntohl(client_addr.sin_addr.s_addr));
      printf("%s ", buf);
      printf("#%hu\n", ntohs(client_addr.sin_port));
    }
    break;
  default:
    printf("unknown remote address family\n");
  }
#endif

  r = close(conn_fd);
  if (r < 0) {
    perror("close() returned error");
    exit(1);
  }

  r = close(socket_fd);
  if (r < 0) {
    perror("close() returned error");
    exit(1);
  }

  printf("Success\n");

  return 0;
}
