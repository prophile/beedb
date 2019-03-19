#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdint.h>
#include <signal.h>

#include "book.h"
#include "port.h"
#include "port_udp.h"
#include "port_pipe.h"

static port main_port;
static book reference_book;

static void handle_signal(int signal) {
    book_close(&reference_book);
    port_close(&main_port);
    exit(0);
}

static uint64_t mid_from_buffer(const char* buffer) {
    return (uint64_t)buffer[0]
         + ((uint64_t)buffer[1] << 8)
         + ((uint64_t)buffer[2] << 16)
         + ((uint64_t)buffer[3] << 24)
         + ((uint64_t)buffer[4] << 32)
         + ((uint64_t)buffer[5] << 40)
         + ((uint64_t)buffer[6] << 48)
         + ((uint64_t)buffer[7] << 56);
}

static void mid_to_buffer(char* buffer, uint64_t mid) {
    buffer[0] = (uint8_t)mid;
    buffer[1] = (uint8_t)(mid >> 8);
    buffer[2] = (uint8_t)(mid >> 16);
    buffer[3] = (uint8_t)(mid >> 24);
    buffer[4] = (uint8_t)(mid >> 32);
    buffer[5] = (uint8_t)(mid >> 40);
    buffer[6] = (uint8_t)(mid >> 48);
    buffer[7] = (uint8_t)(mid >> 56);
}

static bool parse_id(const char* id, uint32_t* page_id, uint32_t* index) {
    int matches = sscanf(id, "%u/%u", page_id, index);
    return matches == 2;
}

static size_t format_id(char* id, uint32_t page_id, uint32_t index) {
    return (size_t)sprintf(id, "%u/%u", page_id, index);
}

int main(int argc, char** argv) {
    int option;
    const char* root = "book";
    const char* bind = "::1";
    const char* namedpipe = NULL;
    uint16_t udp_port = 1213;

    while ((option = getopt(argc, argv, "r:p:b:P:h")) > 0) {
        switch (option) {
            case 'r':
                root = optarg;
                break;
            case 'p':
                udp_port = (uint16_t)atoi(optarg);
                break;
            case 'P':
                namedpipe = optarg;
                break;
            case 'b':
                bind = optarg;
                break;
            case 'h':
            default:
                printf("Usage: %s [-r root] [-b bind] [-p port] [-P pipe] [-h]\n", argv[0]);
                return 0;
        }
    }

    srandomdev();
    mkdir(root, 0755);

    if (namedpipe) {
        int fd = open(namedpipe, O_RDONLY);
        if (fd < 0) {
            fprintf(stderr, "Unable to open named pipe.\n");
            return -1;
        }
        if (!port_open_pipe(&main_port, 1400, fd, '\n')) {
            fprintf(stderr, "Unable to set up named pipe port.\n");
            return -1;
        }
    } else {
        if (!port_open_udp(&main_port, bind, udp_port)) {
            fprintf(stderr, "Unable to open main socket.\n");
            return -1;
        }
    }
    book_init(&reference_book, root);

    signal(SIGTERM, &handle_signal);

    size_t read_buffer_length = main_port.max_length + 1;
    char read_buffer[read_buffer_length];
    char data_buffer[read_buffer_length];

    while (true) {
        void* channel;
        uint32_t page_id, index;
        size_t message_length = port_receive(&main_port, &channel, read_buffer, read_buffer_length);
        printf("Got a message, length = %zu\n", message_length);
        if (message_length < 9) {
            // Malformed or missing message; discard
            port_close_channel(&main_port, channel);
            continue;
        }
        uint64_t message_id = mid_from_buffer(read_buffer);
        if (message_id & (uint64_t)1) {
            // Lower bit is set, which means this is a write
            if (book_put(&reference_book, read_buffer + 8, message_length - 8, &page_id, &index)) {
                mid_to_buffer(data_buffer, message_id & ~(uint64_t)1);
                size_t id_length = format_id(data_buffer + 8, page_id, index);
                port_transmit(&main_port, channel, data_buffer, 8 + id_length);
            }
        } else {
            // Lower bit is cleared, which means this is a read
            if (parse_id(read_buffer + 8, &page_id, &index)) {
                size_t obj_length = book_get(&reference_book, data_buffer + 8, read_buffer_length - 8, page_id, index);

                if (obj_length > 0) {
                    mid_to_buffer(data_buffer, message_id | (uint64_t)1);
                    port_transmit(&main_port, channel, data_buffer, 8 + obj_length);
                }
            }
        }
        port_close_channel(&main_port, channel);
    }
}
