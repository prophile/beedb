#ifndef __INCLUDED_PORT_PIPE_H
#define __INCLUDED_PORT_PIPE_H

#include "port.h"
#include <stdbool.h>

bool port_open_pipe(port* port, size_t maxlen, int fd, char separator);

#endif
