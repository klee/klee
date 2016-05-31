// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --posix-runtime %t1.bc --sym-streams 1 10 > %t.log
// RUN: grep -q "*foo" %t.log
// UN: gcc -g -Dklee_silent_exit=exit %s -o SocketRecv
// UN: %runbout ./SocketRecv %t.klee-out/test000001.bout > %t2.log
// UN: grep -q foo %t2.log

/* Testing that on the server side we can read bytes from an
   established connection */

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

/* simplified version, avoid having to include uclibc */
int strcmp(const char *c1, const char *c2) {
  int i;
  for (i = 0; c1[i] && c2[i]; ++i) {
    if (c1[i] != c2[i])
      return -1;
  }
  if (c1[i] == c2[i])
    return 0;
  return -1;
}

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
  r = accept(socket_fd, (struct sockaddr *)&client_addr, &client_len);
  if (r < 0) {
    perror("accept() returned error");
    exit(1);
  }

  nread = recv(r, buf, 1, 0);
  assert(nread == 1);

  if (buf[0] == '*')
    printf("*");
  else
    klee_silent_exit(0);

  nread = recv(r, buf, sizeof(buf), 0);
  assert(nread == 9);

  if (strcmp(buf, "foo") == 0)
    printf("foo\n");
  else
    klee_silent_exit(0);
  close(r);

  return 0;
}
