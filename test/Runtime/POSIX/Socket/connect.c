// RUN: %llvmgcc %s -g -emit-llvm -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -libc=uclibc --posix-runtime %t.bc -sym-stream 1 2 -sym-datagrams 1 2 > %t.log 2>&1
// RUN: cat %t.log | FileCheck %s

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

int main(int argc, char **argv) {
  
  struct hostent *server;

  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  assert(sockfd >= 0 && "Can't open socket");
  server = gethostbyname("localhost");
  if(server == NULL) {
      printf("No such host\n");
      //CHECK-NOT: No such host
      exit(0);
  }


  unsigned short port;
  klee_make_symbolic(&port, sizeof(port), "port");
  klee_assume(port > 2000 & port < 2002);
  struct sockaddr_in serv_addr = {.sin_family = AF_INET ,
                                  .sin_port = port};
  memcpy( (char *)&serv_addr.sin_addr.s_addr, (char *)server->h_addr, server->h_length);
  printf("Waiting to connect\n"); 

  assert(connect(sockfd,(struct sockaddr *) &serv_addr, sizeof(serv_addr)) >= 0 && "Can't connect");
  printf("connect\n"); 

  int n = write(sockfd, "hello", 5);
  assert(n == 5 && "Couldn't write 5 bytes");
  char buf[3];
  n = read(sockfd, buf, 2);
  printf("Read %d\n", n);
  assert(n == 2 && "Didn't read enough bytes from socket");
  if(buf[0] > 'a' & buf[0] < 'z') 
      printf("Got a small letter\n");
      //CHECK-DAG: Got a small letter
  else 
      printf("Got something else\n");
      //CHECK-DAG: Got something else
  
  close(sockfd);

}
