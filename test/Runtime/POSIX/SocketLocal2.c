// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --posix-runtime %t1.bc 2> %t.log
// RUN: grep -q "accept() returned error: Invalid argument" %t.log

/* Testing that accept fails if performed on a non-listening socket */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <assert.h>
#include <linux/net.h>
#include <errno.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main(int argc, char **argv) {
  struct sockaddr_in server_addr, client_addr;
  int client_len;

  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  // fprintf(stderr, "socket_fd = %d\n", socket_fd);

  if (socket_fd < 0) {
    perror("socket() returned error");
    exit(0);
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(1080);

  int r = bind(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
  if (r < 0) {
    perror("bind() returned error");
    exit(0);
  }

  /* No listen()! */

  client_len = sizeof(client_addr);
  r = accept(socket_fd, (struct sockaddr *)&client_addr, &client_len);
  if (r < 0) {
    perror("accept() returned error");
    exit(0);
  }

  /* should be unreachable */
  printf("Success\n");

  return 0;
}
