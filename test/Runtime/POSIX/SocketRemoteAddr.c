// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --posix-runtime %t1.bc --sym-streams 1 10 > %t.log
// RUN: grep -q Accepting %t.log
// RUN: grep -q Rejecting %t.log
// UN: gcc -g %s -o SocketRemoteAddr
// UN: %runbout ./SocketRemoteAddr %t.klee-out/test000002.bout > %t2.log
// UN: grep -q Rejecting %t2.log
// UN: %runbout ./SocketRemoteAddr %t.klee-out/test000001.bout > %t3.log
// UN: grep -q Accepting %t3.log

/* Testing symbolic remote client addresses */

#include <stdio.h>
#include <string.h>
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
  int nread;
  char buf[256];

  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);

  if (socket_fd < 0) {
    perror("socket() returned error");
    exit(1);
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(1081);

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
#if 0
  printf("sockfd = %d\n", socket_fd);
  printf("&client_addr = %p\n", &client_addr);
  printf("&client_len = %p\n", &client_len);
  printf("client_len = %d\n", client_len);
#endif
  r = accept(socket_fd, (struct sockaddr *)&client_addr, &client_len);
  if (r < 0) {
    perror("accept() returned error");
    exit(1);
  }

  // if (strcmp(inet_ntoa(client_addr.sin_addr), "128.1.43.57") == 0) // this
  // takes forever
  if (client_addr.sin_addr.s_addr == htonl(inet_network("128.1.43.57")))
    printf("Accepting 128.1.43.57\n");
  else
    printf("Rejecting\n");

  close(r);

  return 0;
}
