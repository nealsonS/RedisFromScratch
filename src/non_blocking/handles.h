#include <vector>
#include <sys/types.h>

// a place to store states of connections
struct Conn {
    int fd = -1;
    bool want_read = false;
    bool want_write = false;
    bool want_close = false;

    // buffers
    std::vector<uint8_t> incoming; // buffers for data to be parsed
    std::vector<uint8_t> outgoing; // buffers for data to be written
};

void fd_set_nonblock(int fd);

Conn* handle_accept(int fd);

void buf_append(std::vector<uint8_t> &buf, const u_int8_t* data, size_t len);
void buf_consume(std::vector<uint8_t> &buf, size_t n);

bool try_one_request(Conn *conn);
void handle_read(Conn *conn);

void handle_write(Conn *conn);
