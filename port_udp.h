#ifndef __INCLUDED_PORT_UDP_H
#define __INCLUDED_PORT_UDP_H

#include "port.h"
#include <stdint.h>
#include <stdbool.h>

bool port_open_udp(port* port, const char* bind, uint16_t portno);

#endif
