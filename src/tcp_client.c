#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "io.h"

const size_t header_size = 4;
const size_t k_max_msg = 4096;

static int32_t query(int fd, const char *text){
  uint32_t len = (uint32_t)strlen(text);
  if (len > k_max_msg){
    return -1;
  }

  char wbuf[header_size + k_max_msg];

  // write the write buffer
  memcpy(wbuf, &len, header_size);
  memcpy(&wbuf[4], text, len);

  // write to socket
  int32_t write_err = write_full(fd, wbuf, header_size+len);
  if (write_err){
    return write_err;
  }

  // read header
  char rbuf[header_size+k_max_msg];
  int errno=0;
  int32_t err = read_full(fd, rbuf, header_size);
  if (err){
    printf(errno==0 ? "EOF": "read() header error");
  }
  
  // write header to the len
  memcpy(&len, rbuf, header_size);
  if (len>k_max_msg){
    printf("msg too long");
    return -1;
  }

  // read body
  err = read_full(fd, &rbuf[4], len);
  if (err){
    printf(errno==0 ? "EOF": "read() body error");
  }

  printf("server says: %s\n", &rbuf[4]);

}
int main(){
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  
  struct sockaddr_in addr = {};
  addr.sin_family = AF_INET;
  addr.sin_port = ntohs(1234);

  // INADDR_LOOPBACK is essentially 127.0.0.1 in in_addr_t
  addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK);
  int rv = connect(fd, (const struct sockaddr*)&addr, sizeof(addr));
  // printf("now connecting to\nIP: %")

  // char msg[] = "hello";
  // write(fd, msg, strlen(msg));

  // char rbuf[64] = {};
  // ssize_t n = read(fd, rbuf, sizeof(rbuf) - 1);
  int32_t err = query(fd, "hello1");
  if (err){
    goto L_DONE;
  }
  err = query(fd, "hello2");
  if (err){
    goto L_DONE;
  }

  // printf("server says: %s\n", rbuf);
  L_DONE:
    close(fd);
    return 0;
}