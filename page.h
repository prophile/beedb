#ifndef __INCLUDED_PAGE_H
#define __INCLUDED_PAGE_H

#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>

const static size_t PAGE_SIZE = 1024 * 1024 * 4;
const static uint32_t PAGE_NONE = (uint32_t)-1;

typedef struct _page {
    void* region;
    bool is_write : 1;
    int fd : 31;
    uint32_t page_id;
} page;

static inline void page_init(page* page) {
    page->region = NULL;
    page->page_id = PAGE_NONE;
    page->fd = -1;
    page->is_write = false;
}

void page_close(page* page);
bool page_open_read(const char* root, page* page, uint32_t page_id);
bool page_open_write(const char* root, page* page, uint32_t page_id);

#endif

