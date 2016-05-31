// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --posix-runtime %t1.bc > %t.log
// RUN: grep -q Success %t.log
// UN: gcc -g %s -o SocketAccept2
// UN: %runbout ./SocketAccept2 %t.klee-out/test000001.bout > %t2.log
// UN: grep -q Success %t2.log

/* Testing that accept fails gracefully if performed more times than provided
 * symbolic streams */

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
  int conn_fd, conn_fd2;

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
  if (conn_fd >= 0) {
    fprintf(stderr, "accept() (2nd time) returned success\n");
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
