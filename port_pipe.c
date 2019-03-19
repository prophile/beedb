#include <stddef.h>
#include <string.h>
#include <unistd.h>

#include "port_pipe.h"

static inline void* embed_fd_and_separator(int fd, char separator) {
    ptrdiff_t embedded = (ptrdiff_t) fd;
    embedded |= ((ptrdiff_t)(unsigned char)separator) << 32;
    return (void*)embedded;
}

static inline int extract_fd(void* ptr) {
    return (int)ptr;
}

static inline char extract_separator(void* ptr) {
    ptrdiff_t embedded = (ptrdiff_t)ptr;
    embedded >>= 32;
    return (char)embedded;
}

static size_t receive_pipe(void** channel, void* buf, size_t len, void* userdata) {
    int fd = extract_fd(userdata);
    char separator = extract_separator(userdata);
    ssize_t read_result = read(fd, buf, len);
    if (read_result <= 0) {
        return 0;
    }
    *channel = NULL;
    // If the last character was the separator, shorten the result by 1
    if (((char*)buf)[read_result - 1] == separator) {
        --read_result;
    }
    return (size_t)read_result;
}

static void transmit_pipe(const void* channel, const void* buf, size_t len, void* userdata) {
    int fd = extract_fd(userdata);
    char separator = extract_separator(userdata);
    char write_buffer[len + 1];
    memcpy(write_buffer, buf, len);
    write_buffer[len] = separator;
    write(fd, write_buffer, len + 1);
}

static void close_channel_pipe(void* channel, void* userdata) {
    (void)channel;
    (void)userdata;
}

static void close_pipe(void* userdata) {
    (void)userdata;
}

bool port_open_pipe(port* port, size_t maxlen, int fd, char separator) {
    port->receive = &receive_pipe;
    port->transmit = &transmit_pipe;
    port->close_channel = &close_channel_pipe;
    port->close = &close_pipe;
    port->max_length = maxlen;
    port->userdata = embed_fd_and_separator(fd, separator);
    return true;
}
