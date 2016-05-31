// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --posix-runtime %t1.bc --sym-streams 1 36 > %t.log
// RUN: grep -q Success %t.log
// UN: gcc -g %s
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

int main(int argc, char **argv) {
  struct sockaddr_in server_addr, client_addr;
  socklen_t client_len;
  int listen_fd, conn_fd, r;
  char buf[36], *pbuf = buf;
  struct iovec iov;
  struct msghdr msg = {NULL, 0, &iov, 1, NULL, 0, 0};

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(1080);

  PRINT("Trying socket()... ");
  listen_fd = socket(PF_INET, SOCK_STREAM, 0);
  EXPECT_SUCCESS(listen_fd);

  PRINT("Trying bind()... ");
  r = bind(listen_fd, (const struct sockaddr *)&server_addr,
           sizeof(server_addr));
  EXPECT_SUCCESS(r);

  PRINT("Trying listen()... ");
  r = listen(listen_fd, 1);
  EXPECT_SUCCESS(r);

  PRINT("Trying accept()... ");
  client_len = sizeof(client_addr);
  conn_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len);
  EXPECT_SUCCESS(conn_fd);

  PRINT("Trying read()... ");
  r = read(conn_fd, pbuf, 10);
  EXPECT_EQUAL(r, 10);
  pbuf += 10;

  PRINT("Trying recvfrom()... ");
  client_len = sizeof(client_addr);
  r = recvfrom(conn_fd, pbuf, 10, 0, (struct sockaddr *)&client_addr,
               &client_len);
  EXPECT_EQUAL(r, 10);
  pbuf += 10;

  PRINT("Trying recvmsg()... ");
  iov.iov_base = pbuf;
  iov.iov_len = 10;
  r = recvmsg(conn_fd, &msg, 0);
  EXPECT_EQUAL(r, 10);
  pbuf += 10;

  PRINT("Trying recv()... ");
  r = recv(conn_fd, pbuf, 10, 0);
  EXPECT_EQUAL(r, 6);
  pbuf += 6;

  PRINT("Trying recv()... ");
  r = recv(conn_fd, pbuf, 10, 0);
  EXPECT_EQUAL(r, 0);

  PRINT("Trying send()... ");
  r = send(conn_fd, buf, pbuf - buf, 0);
  EXPECT_EQUAL(r, pbuf - buf);

  PRINT("Trying close()... ");
  r = close(conn_fd);
  EXPECT_SUCCESS(r);

  PRINT("Trying close()... ");
  r = close(listen_fd);
  EXPECT_SUCCESS(r);

  printf("Success\n");
  return EXIT_SUCCESS;
}
