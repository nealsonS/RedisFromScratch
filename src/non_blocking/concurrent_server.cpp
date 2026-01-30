#include <unistd.h>
#include <fcntl.h>
#include <vector>
#include <poll.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>

#include "./handles.h"

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




int main() {
    const u_int16_t port = 1234;
    const char ip_addr[] = "127.0.0.1";

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    // bind
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if(inet_pton(AF_INET, ip_addr, &(addr.sin_addr)) <= 0){
        perror("error ip");
        return 1;
    }

    //listen
    if (bind(fd, (sockaddr*)&addr, sizeof(addr)) < 0) { perror("bind"); return 1; }
    fd_set_nonblock(fd);

    if (listen(fd, SOMAXCONN) < 0) { perror("listen"); return 1; }

    // vector to store sockets to be turned to connections
    // in the next loop
    std::vector<Conn*> fd2Conn;
    
    // event loop
    std::vector<struct pollfd> poll_args;
    while(true){
        // resets the list of poll arguments (to be inputted) 
        poll_args.clear();

        // put listening sockets in the first position of poll_args
        struct pollfd pfd = {fd, POLLIN, 0};
        poll_args.push_back(pfd);

        // now fill in connection sockets
        // this is building the poll sockets for every iteration
        for (Conn* conn : fd2Conn){
            if(!conn){
                continue;
            }
            // read sockets
            struct pollfd pfd = {conn->fd, POLLERR, 0};

            // set based on application's intent
            // so if connection wants read, set to read
            if (conn->want_read){
                pfd.events |= POLLIN;
            }
            if (conn->want_write){
                pfd.events |= POLLOUT;
            }
            poll_args.push_back(pfd);
        }

        // now wait until a socket is ready
        // ready means that a socket is ready to write/read
        // since timeout is -1, blocks until events occur
        int rv = poll(poll_args.data(), (nfds_t) poll_args.size(), -1);

        // sometimes poll returns -1 and errno == EINTR
        // not an error, but unix interrupted poll
        if (rv< 0 && errno == EINTR){
            continue;
        }
        if (rv < 0){
            continue;
        }

        // handle the listening socket
        // if the listening socket gets incoming requests
        // handle_accept creates the connection based on the incoming socket
        // whether it's read or write
        // and creates the conn struct accordingly
        if (poll_args[0].revents) {
            if (Conn *conn = handle_accept(fd)){
                
                // add the connection (fd and its state)
                // to the last entry of the vector
                // resize if not enough
                if (fd2Conn.size() <= (size_t)conn->fd){
                    fd2Conn.resize(conn->fd + 1);
                }
                fd2Conn[conn->fd] = conn;
            }
        }

        // handle connection sockets
        for (size_t i=1; i < poll_args.size(); ++i){ // pre-add to skip first socket

            // get ready status of socket in poll_args
            // ready is a bitmask of events
            u_int32_t ready = poll_args[i].revents;
            Conn *conn = fd2Conn[poll_args[i].fd];

            // this basically checks if the flag for POLLIN
            // is 1/true in ready
            // since ready is a bitmask
            // bitwise AND basically results in not 0
            // which results in true
            // this is called a bitwise AND check
            if (ready & POLLIN){
                handle_read(conn);
            }

            if (ready & POLLOUT){
                handle_write(conn);
            }

            // handle closing connection
            if ((ready & POLLERR) || conn->want_close){
                (void)close(conn->fd);
                fd2Conn[conn->fd] = NULL;
                delete conn;
            }
        }
    }

}