#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>

#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "handles.h"

// #include <unistd.h>

#define HEADER_SIZE 4
#define MAX_MSG 4096

// // a place to store states of connections
// struct Conn {
//     int fd = -1;
//     bool want_read = false;
//     bool want_write = false;
//     bool want_close = false;

//     // buffers
//     std::vector<uint8_t> incoming; // buffers for data to be parsed
//     std::vector<uint8_t> outgoing; // buffers for data to be written
// };

void fd_set_nonblock(int fd) {
    /*
    1. Get flags
    2. Set O_NONBLOCK into the flags
    3. set the flags
    */
    
    int flags = fcntl(fd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(fd, F_SETFL, flags);
    // TODO: handle error
}

Conn* handle_accept(int fd){
    struct sockaddr_in client_addr = {};
    socklen_t addrlen = sizeof(client_addr);

    int connfd = accept(fd, (struct sockaddr*)&client_addr, &addrlen);
    
    // if failed
    if (connfd < 0){
        return NULL;
    }

    // set new socket to non blocking
    fd_set_nonblock(connfd);

    // create a `Struct Conn`
    Conn *conn = new Conn();
    conn->fd = connfd;
    conn->want_read=true; 
    return conn;
}

// append to the back
void buf_append(std::vector<uint8_t> &buf, const uint8_t *data, size_t len) {
  buf.insert(buf.end(), data, data + len);
}

// remove from front
void buf_consume(std::vector<uint8_t> &buf, size_t n) {
  buf.erase(buf.begin(), buf.begin() + n);
}

// try to parse a request
bool try_one_request(Conn *conn) {
  // 3. Try to parse accumulated buffer
  // message header first
  if (conn->incoming.size() < HEADER_SIZE) {
    return false; // want read more data
  }

  uint32_t len = 0;
  memcpy(&len, conn->incoming.data(), HEADER_SIZE);
  // if (len > MAX_MSG) {
  //   conn->want_close = true;
  //   return false;
  // }

  // if needs to allocate more
  if (HEADER_SIZE + len > conn->incoming.size()) {
    return false;
  }

  // get request
  const uint8_t *request = &conn->incoming[HEADER_SIZE];

  // 4. process parsed message
  buf_append(conn->outgoing, (const uint8_t *)&len, HEADER_SIZE);
  buf_append(conn->outgoing, request, (size_t)len);

  // 5. remove message from `Conn::incoming`
  buf_consume(conn->incoming, HEADER_SIZE + len);
  return true; // success
}

void handle_read(Conn *conn) {
    // 1. do a non-blocking read
    // header size + max body
    uint8_t buf[64*1024];
    ssize_t rv = read(conn->fd, buf, sizeof(buf));

    // EOF (rv==0) or IO error (rv<0)
    // if errno == EAGAIN, that means it's normal
    // not error
    if (rv < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return;

    if (rv <= 0){
        conn->want_close=true;
        return;
    }

    // 2. add new data to `Conn::incoming` buffer
    buf_append(conn->incoming, buf, (size_t)rv);

    // 3. try to parse accumulated buffer
    // 4. process parsed message
    // 5. if successful, remove message from Conn::incoming
    while (try_one_request(conn)) {}

    // if have response to write
    // switch intent
    if (conn->outgoing.size() > 0) {
      conn->want_read = false;
      conn->want_write = true;
      return handle_write(conn);
    }
}

void handle_write(Conn *conn) {
  assert(conn->outgoing.size() > 0);
  ssize_t rv = write(conn->fd, conn->outgoing.data(), conn->outgoing.size());

  // if IO error
  if (rv < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return;

  if (rv < 0) {
    conn->want_close = true;
    return;
  }

  // remove from buffer
  buf_consume(conn->outgoing, (size_t)rv);

    // if have response to write
    // switch intent
    if (conn->outgoing.empty()) {
      conn->want_write = false;
      conn->want_read = true;
    }
}