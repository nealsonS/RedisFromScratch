#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>

#include "./io.h"

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
// one write one read
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

const size_t k_max_msg = 4096;
const size_t header_size = 4;

static int32_t one_request(int connfd){
  char rbuf[header_size + k_max_msg];
  int errno = 0;

  // parse header and get msg_length
  int32_t err = read_full(connfd, rbuf, header_size);

  if (err) {
    printf(errno==0 ? "EOF" : "read() error");
    return err;
  }

  // set msg_length as len
  uint32_t len = 0;
  memcpy(&len, rbuf, header_size);

  if(len > k_max_msg){
    printf("msg too long");
    return -1;
  }

  // read the next N bytes of the socket request
  err = read_full(connfd, &rbuf[header_size], len);
  if(err){
    printf("read() body error");
    return err;
  }

  printf("client says: %s\n", &rbuf[header_size]);

  // reply using same protocol
  const char reply[] = "world";
  char wbuf[header_size + sizeof(reply)];
  len = (uint32_t)strlen(reply);

  memcpy(wbuf, &len, header_size);
  memcpy(&wbuf[4], &reply, len);

  return write_full(connfd, wbuf, header_size + len);
}


int main(){
  const char* ip_addr = "0.0.0.0";
  const u_int16_t port = 1234;

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
  addr.sin_port = htons(port); // store using Endian numbers
  // addr.sin_addr.s_addr = htonl(ip_addr); // wildcard of 0.0.0.0

    // convert ip string to binary in a structure
  if (inet_pton(AF_INET, ip_addr, &(addr.sin_addr)) <= 0) {
      perror("inet_pton failed");
      return 1;
  }

  // bind socket with address
  int rv = bind(fd, (const struct sockaddr*) &addr, sizeof(addr));

  // if (rv) { die("bind()"); }
  
  // handles TCP handshakes and puts connections in a queue
  // SOMAXCONN basically size of queue and is 4096 on Linux
  rv = listen(fd, SOMAXCONN);
  // if (rv) {die("listen()");}
  printf("Now listening at:\nIP: %s\nPort: %u\nProtocol: %s", ip_addr, port, "tcp");

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
    // do_something(connfd);

    // process in a for loop (basically one request at a time)
    while(1){
      int32_t err = one_request(connfd);
    }
    close(fd);
  }

  return 0;
}