// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --posix-runtime %t1.bc > %t.log
// RUN: grep -q "Success" %t.log

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

  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  // fprintf(stderr, "socket_fd = %d\n", socket_fd);

  if (socket_fd < 0) {
    perror("socket() returned error");
    exit(1);
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
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

  r = close(socket_fd);
  if (r < 0) {
    perror("close() returned error");
    exit(1);
  }

  printf("Success\n");

  return 0;
}
