// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --posix-runtime %t1.bc --sym-datagrams 1 16 --fill-datagrams 0 set 4 0xAA --fill-datagrams 4 copy {okay\\n\\0\\7\\37\\007\\xa\\x7f.} --sym-streams 1 16 --fill-streams 0 set 4 0xBB --fill-streams 4 copy {\\xaokay\\n\\007\\37\\7\\0\\x7f.} > %t.log
// RUN: test -f %t.klee-out/test000001.ktest
// RUN: not test -f %t.klee-out/test000002.ktest
// RUN: grep -q Success %t.log

/* Testing concretization works correctly */

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

/* simplified version, avoid having to include uclibc */
int memcmp(const void *v1, const void *v2, size_t n) {
  int i;
  const char *c1 = (const char *)v1;
  const char *c2 = (const char *)v2;

  for (i = 0; i < n; ++i) {
    if (c1[i] != c2[i])
      return -1;
  }
  return 0;
}

int main(int argc, char **argv) {
  struct sockaddr_in server_addr, client_addr;
  socklen_t client_len;
  int nread;
  char buf[16];

  int ufd = socket(AF_INET, SOCK_DGRAM, 0);
  if (ufd < 0) {
    perror("socket() returned error");
    exit(1);
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(1080);

  int r = bind(ufd, (const struct sockaddr *)&server_addr, sizeof(server_addr));
  if (r < 0) {
    perror("bind() returned error");
    exit(1);
  }

  nread = recv(ufd, buf, sizeof(buf), 0);
  if (nread < 0) {
    perror("recv() returned error");
    exit(1);
  }

  assert(nread == 16);
  if (klee_is_symbolic(buf[0])) {
    printf("Received symbolic data. Failed\n");
  }
  assert(memcmp(buf + 0, "\xAA\xAA\xAA\xAA", 4) == 0);
  assert(memcmp(buf + 4, "okay\n\0\7\37\007\xa\x7f.", 12) == 0);

  close(ufd);

  int lfd = socket(AF_INET, SOCK_STREAM, 0);
  if (lfd < 0) {
    perror("socket() returned error");
    exit(1);
  }

  r = bind(lfd, (const struct sockaddr *)&server_addr, sizeof(server_addr));
  if (r < 0) {
    perror("bind() returned error");
    exit(1);
  }

  r = listen(lfd, 5);
  if (r < 0) {
    perror("listen() returned error");
    exit(1);
  }

  client_len = sizeof(client_addr);
  int cfd = accept(lfd, (struct sockaddr *)&client_addr, &client_len);
  if (cfd < 0) {
    perror("accept() returned error");
    exit(1);
  }

  nread = recv(cfd, buf, sizeof(buf), 0);
  if (nread < 0) {
    perror("recv() returned error");
    exit(1);
  }

  assert(nread == 16);
  assert(memcmp(buf + 0, "\xBB\xBB\xBB\xBB", 4) == 0);
  assert(memcmp(buf + 4, "\xaokay\n\007\37\7\0\x7f.", 12) == 0);

  close(cfd);
  close(lfd);

  printf("Success\n");

  return 0;
}
