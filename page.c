#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

#include "page.h"

void page_close(page* page) {
    if (page->page_id == PAGE_NONE) {
        // already closed
        return;
    }

    if (page->is_write) {
        msync(page->region, PAGE_SIZE, MS_SYNC);
    }
    munmap(page->region, PAGE_SIZE);
    close(page->fd);
    page_init(page);
}

static int open_page(const char* root, uint32_t page_id, int oflag) {
    // unsigned 32-integers can be up to 10 characters long; the buffer
    // can contain the root + slash + ID + ".page" + null
    char path_buf[strlen(root) + 17];
    sprintf(path_buf, "%s/%u.page", root, page_id);
    return open(path_buf, oflag, 0600);
}

bool page_open_read(const char* root, page* page, uint32_t page_id) {
    if (page_id == PAGE_NONE) {
        return false;
    }
    int fd = open_page(root, page_id, O_RDONLY);
    if (fd == -1)
        return false;
    void* region = mmap(NULL, PAGE_SIZE, PROT_READ, MAP_SHARED, fd, 0);
    if (region == NULL) {
        close(fd);
        return false;
    }
    page->region = region;
    page->page_id = page_id;
    page->fd = fd;
    page->is_write = false;
    return true;
}

bool page_open_write(const char* root, page* page, uint32_t page_id) {
    if (page_id == PAGE_NONE) {
        return false;
    }
    int fd = open_page(root, page_id, O_RDWR | O_CREAT);
    if (fd == -1)
        return false;
    // preallocate the page size
    int res = ftruncate(fd, PAGE_SIZE);
    if (__builtin_expect(res != 0, false)) {
        // write zeros to the file
        char zero_buffer[1024];
        memset(zero_buffer, 0, sizeof(zero_buffer));
        for (int i = 0; i < PAGE_SIZE / sizeof(zero_buffer); ++i) {
            ssize_t written = write(fd, zero_buffer, sizeof(zero_buffer));
            if (written != sizeof(zero_buffer)) {
                close(fd);
                return false;
            }
        }
    }
    void* region = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (region == NULL) {
        close(fd);
        return false;
    }
    page->region = region;
    page->page_id = page_id;
    page->fd = fd;
    page->is_write = false;
    return true;
}

