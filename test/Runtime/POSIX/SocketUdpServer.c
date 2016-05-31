// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --posix-runtime %t1.bc --sym-datagrams 4 10 > %t.log
// RUN: grep -q Success %t.log
// UN: gcc -g -Dklee_silent_exit=exit %s
// UN: %runbout ./a.out %t.klee-out/test000001.bout > %t2.log
// UN: grep -q Success %t2.log

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

#define PRINT(msg) (fputs(msg, stdout), fflush(stdout))

#define EXPECT_SUCCESS(rc)                                                     \
  if ((rc) < 0) {                                                              \
    printf("failed: %s\n", strerror(errno));                                   \
    return EXIT_FAILURE;                                                       \
  } else                                                                       \
  printf("returns %d\n", (rc))

#define EXPECT_EQUAL(rc, val)                                                  \
  if ((rc) < 0) {                                                              \
    printf("failed: %s\n", strerror(errno));                                   \
    return EXIT_FAILURE;                                                       \
  } else if ((rc) != (val)) {                                                  \
    printf("returns %d, expected %d\n", (rc), (val));                          \
    return EXIT_FAILURE;                                                       \
  } else                                                                       \
  printf("returns %d\n", (rc))

#define ASSUME_EQUAL_STR(s1, s2)                                               \
  if (strcmp(s1, s2) != 0)                                                     \
    klee_silent_exit(0);

#define NBYTES 4

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
  socklen_t client_len;
  int fd, conn_fd, r;
  char buf[4 * NBYTES], *pbuf = buf;
  struct iovec iov;
  struct msghdr msg = {NULL, 0, &iov, 1, NULL, 0, 0};

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(1080);

  PRINT("Trying socket()... ");
  fd = socket(PF_INET, SOCK_DGRAM, 0);
  EXPECT_SUCCESS(fd);

  PRINT("Trying bind()... ");
  r = bind(fd, (const struct sockaddr *)&server_addr, sizeof(server_addr));
  EXPECT_SUCCESS(r);

  PRINT("Trying recvfrom()... ");
  client_len = sizeof(client_addr);
  r = recvfrom(fd, pbuf, NBYTES, 0, (struct sockaddr *)&client_addr,
               &client_len);
  EXPECT_EQUAL(r, NBYTES);
  ASSUME_EQUAL_STR(pbuf, "foo");
  pbuf += NBYTES;

  PRINT("Trying recvmsg()... ");
  iov.iov_base = pbuf;
  iov.iov_len = NBYTES;
  r = recvmsg(fd, &msg, 0);
  EXPECT_EQUAL(r, NBYTES);
  ASSUME_EQUAL_STR(pbuf, "bar");
  pbuf += NBYTES;

  PRINT("Trying recv()... ");
  r = recv(fd, pbuf, NBYTES, 0);
  EXPECT_EQUAL(r, NBYTES);
  ASSUME_EQUAL_STR(pbuf, "baz");
  pbuf += NBYTES;

  PRINT("Trying read()... ");
  r = read(fd, pbuf, NBYTES);
  EXPECT_EQUAL(r, NBYTES);
  ASSUME_EQUAL_STR(pbuf, "qux");
  pbuf += NBYTES;

  PRINT("Trying sendto()... ");
  r = sendto(fd, buf, pbuf - buf, 0, (const struct sockaddr *)&client_addr,
             client_len);
  EXPECT_EQUAL(r, pbuf - buf);

  PRINT("Trying close()... ");
  r = close(fd);
  EXPECT_SUCCESS(r);

  printf("Success\n");
  return EXIT_SUCCESS;
}
