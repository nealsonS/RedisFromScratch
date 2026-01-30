#include <unistd.h>
#include <fcntl.h>

/*
PSEUDOCODE: 
while running:
    want_read = [...]           # socket fds
    want_write = [...]          # socket fds
    can_read, can_write = wait_for_readiness(want_read, want_write) # blocks!
    for fd in can_read:
        data = read_nb(fd)      # non-blocking, only consume from the buffer
        handle_data(fd, data)   # application logic without IO
    for fd in can_write:
        data = pending_data(fd) # produced by the application
        n = write_nb(fd, data)  # non-blocking, only append to the buffer
        data_written(fd, n)     # n <= len(data), limited by the available space


Readiness notification: Wait for multiple sockets, return when one or more are ready. “Ready” means the read buffer is not empty or the write buffer is not full.
Non-blocking read: Assuming the read buffer is not empty, consume from it.
Non-blocking write: Assuming the write buffer is not full, put some data into it.

*/

static void fd_set_nonblock(int fd) {
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