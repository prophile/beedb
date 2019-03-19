#ifndef __INCLUDED_PORT_H
#define __INCLUDED_PORT_H

#include <unistd.h>
#include <stdbool.h>

typedef struct _port {
    size_t (*receive)(
        void** channel,
        void* buffer,
        size_t maxlen,
        void* userdata
    );
    void (*transmit)(
        const void* channel,
        const void* buffer,
        size_t length,
        void* userdata
    );
    void (*close_channel)(
        void* channel,
        void* userdata
    );
    void (*close)(void* userdata);
    size_t max_length;
    void* userdata;
} port;

inline static size_t port_receive(
    port* port,
    void** channel,
    void* buffer,
    size_t maxlen
) {
    return port->receive(channel, buffer, maxlen, port->userdata);
}

inline static void port_transmit(
    port* port,
    const void* channel,
    const void* buffer,
    size_t length
) {
    return port->transmit(channel, buffer, length, port->userdata);
}

inline static void port_close_channel(
    port* port,
    void* channel
) {
    port->close_channel(channel, port->userdata);
}

inline static void port_close(port* port) {
    port->close(port->userdata);
}

#endif
