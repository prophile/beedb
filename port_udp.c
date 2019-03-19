#include <netdb.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "port_udp.h"

static inline void* embed_fd(int fd) {
    return (void*)((ptrdiff_t)fd);
}

static inline int extract_fd(void* ptr) {
    return (int)ptr;
}

struct _channel_udp {
    struct sockaddr_in6 address;
};

static size_t receive_udp(void** channel, void* buf, size_t len, void* userdata) {
    int fd = extract_fd(userdata);
    struct sockaddr_in6 address_buf;
    socklen_t address_length = sizeof(address_buf);
    ssize_t response = recvfrom(fd, buf, len, 0, (struct sockaddr*)&address_buf, &address_length);
    if (response <= 0) {
        return 0;
    }

    if (address_length != sizeof(address_buf)) {
        // This shouldn't happen, but it means we received an address of
        // the wrong shape.
        return 0;
    }

    void* channel_buffer = malloc(sizeof(address_buf));
    memcpy(channel_buffer, &address_buf, sizeof(address_buf));
    *channel = channel_buffer;
    return (size_t)response;
}

static void transmit_udp(const void* channel, const void* buf, size_t len, void* userdata) {
    int fd = extract_fd(userdata);
    struct sockaddr_in6* address = (struct sockaddr_in6*)channel;
    sendto(fd, buf, len, 0, (const struct sockaddr*)address, sizeof(*address));
}

static void close_channel_udp(void* channel, void* userdata) {
    (void)userdata;
    free(channel);
}

static void close_udp(void* userdata) {
    int fd = extract_fd(userdata);
    close(fd);
}

#if defined(SO_REUSEPORT_LB)
#define ENABLE_REUSEPORT
#define REUSEPORT_OPTION SO_REUSEPORT_LB
#elif defined(SO_REUSEPORT)
#define ENABLE_REUSEPORT
#define REUSEPORT_OPTION SO_REUSEPORT
#endif

bool port_open_udp(port* port, const char* bind_addr, uint16_t portno) {
    int res;
    struct protoent* udp = getprotobyname("udp");
    struct sockaddr_in6 address;

    if (!udp) {
        // Couldn't look up protoent for UDP
        return false;
    }

    struct in6_addr base_address;

    if (bind_addr == NULL) {
        base_address = in6addr_any;
    } else {
        res = inet_pton(AF_INET6, bind_addr, &base_address);

        if (res != 1) {
            return false;
        }
    }

    address.sin6_len = sizeof(address);
    address.sin6_family = AF_INET6;
    address.sin6_flowinfo = 0;
    address.sin6_port = htons(portno);
    address.sin6_addr = base_address;

    int fd = socket(PF_INET6, SOCK_DGRAM, udp->p_proto);

    if (fd == -1) {
        return false;
    }

#ifdef ENABLE_REUSEPORT
    res = setsockopt(fd, SOL_SOCKET, REUSEPORT_OPTION, &(int){ 1 }, sizeof(int));
    if (res != 0) {
        close(fd);
        return false;
    }
#endif

    res = bind(fd, (struct sockaddr*)&address, sizeof(address));
    if (res == -1) {
        close(fd);
        return false;
    }

    port->receive = &receive_udp;
    port->transmit = &transmit_udp;
    port->close_channel = &close_channel_udp;
    port->close = &close_udp;
    port->max_length = 1400;
    port->userdata = embed_fd(fd);
    return true;
}
