#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>


// PSEUDOCODE

// create a handle for the kernel
// fd = socket()

// bind the socket with the IP:Port combination
// bind(fd, address)

// accept TCP connections
// listen(fd)

// wait for connections
// while True:

//     when request comes in, accept it
//     conn_fd = accept(fd)
//     do_something_with(conn_fd)
//     close(conn_fd)

// do something function
static void do_something(int connfd){
  char rbuf[64] = {};

  // read RBUF bytes from file descriptor
  ssize_t n = read(connfd, rbuf, sizeof(rbuf) - 1);

  // if fails
  if (n<0){
    printf("read() error");
    return;
  }
  printf("client says %s\n", rbuf);

  char wbuf[] = "world";
  write(connfd, wbuf, strlen(wbuf));

}

int main(){

  // create a TCP socket using IPv4 (AF_INET)
  // use TCP using SOCK_STREAM
  // 
  int fd = socket(AF_INET, SOCK_STREAM, 0);

  int val = 1;

  // establish socket options
  // SOL_SOCKET establish what level the option is at
  // always set SO_REUSEADDR as 1
  // because the ip:port goes to time_wait mode after it ends
  // so, you can't re-use it straight after
  // so setting it to 1, makes servers able to bind to it straight after it ends
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

  // binding to an address
  struct sockaddr_in addr = {};
  addr.sin_family = AF_INET;

  // uses endian numbers because our CPU is little endian
  // but networking needs Big-endian numbers
  // if left unconverted, the ports and ips will be backwards
  addr.sin_port = htons(1234); // store using Endian numbers
  addr.sin_addr.s_addr = htonl(0); // wildcard of 0.0.0.0

  // bind socket with address
  int rv = bind(fd, (const struct sockaddr*) &addr, sizeof(addr));

  // if (rv) { die("bind()"); }
  
  // handles TCP handshakes and puts connections in a queue
  // SOMAXCONN basically size of queue and is 4096 on Linux
  rv = listen(fd, SOMAXCONN);
  // if (rv) {die("listen()");}

  while (1){
    // accept
    struct sockaddr_in client_addr = {};
    socklen_t addrlen = sizeof(client_addr);

    // connection socket
    int connfd = accept(fd, (struct sockaddr*) &client_addr, &addrlen);

    // if failed
    if (connfd < 0) {
      continue; //error
    }
    do_something(connfd);
    close(fd);
  }

  return 0;
}