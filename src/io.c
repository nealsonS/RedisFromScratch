#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

// func to read in a loop
// bcos incoming requests can be less than buffer size
// because tcp might not give all request data at once
// but it's split into packets
// so EOF might not even reach in one request
int32_t read_full(int fd, char* buf, size_t n) {
  while (n > 0){

    // read n bytes into the buffer size
    // returns how many bytes were read
    ssize_t rv = read(fd, buf, n);
    
    // if EOF
    // handle edge case where 0 bytes read but
    // not EOF, just read is interrupted
    if (rv <= 0 & errno != EINTR) {
      return -1; //error or unexpected EOF
    }
    if (rv == -1 & errno == EINTR){
      continue;
    }

    // check that we only read n bytes (or less)
    assert((size_t)rv <= n);
    
    // read only what's left
    n -= (size_t)rv;
    // seek the buffer pointer by how much was read
    buf += rv;
  }
  return 0;
}

int32_t write_full(int fd, char* buf, size_t n){
  while(n>0){
  // returns how many bytes was written
  ssize_t rv = write(fd, buf, n);

  if (rv <= 0){
    return -1;
  }

  assert((size_t)rv <= n);

  // write only what's left
  n -= (size_t)rv;

  // seek the buffer pointer by how much was written
  // to only write what's left of the bufer
  buf += rv;
  }
  return 0;
}