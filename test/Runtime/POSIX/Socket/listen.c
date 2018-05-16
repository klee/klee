// RUN: %llvmgcc %s -g -emit-llvm -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -libc=uclibc --posix-runtime %t.bc -sym-streams 1 1 &> %t.log 
// RUN: cat %t.log | FileCheck %s

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

int main(int argc, char **argv) {
  unsigned short port;
  klee_make_symbolic(&port, sizeof(port), "port");
  struct sockaddr_in serv_addr = {.sin_family = AF_INET , 
                                  .sin_addr.s_addr = INADDR_ANY, 
                                  .sin_port = port};
  struct sockaddr_in cli_addr;
  socklen_t clilen = sizeof(cli_addr);

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  assert(sockfd >= 0 && "Can't open socket");
  assert(bind(sockfd,(struct sockaddr *) &serv_addr, sizeof(serv_addr)) >= 0 && "Can't bind");
  listen(sockfd, 5);
  int newsockfd = accept(sockfd,(struct sockaddr *) &cli_addr, &clilen);

  char buf[3];
  int n = read(newsockfd, buf, 2);
  assert(n == 1 && "Didn't read enough bytes from socket");
  if(buf[0] > 'a' & buf[0] < 'z') 
      printf("Got a small letter\n");
      //CHECK-DAG: Got a small letter
  else 
      printf("Got something else\n");
      //CHECK-DAG: Got something else

  close(sockfd);
  close(newsockfd);
}
