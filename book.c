#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "book.h"
#include "page.h"

static uint32_t pick_random_page(void) {
    return random() % BOOK_PAGE_COUNT;
}

static uint32_t initial_page_number(void) {
    // random() has enough entropy to fill this.
    uint32_t page = random();
    if (page == 0 || page == PAGE_NONE) {
        return 4;  // Chosen by fair dice roll, guaranteed to be random
    }
    return page;
}

void book_init(book* book, const char* root) {
    for (int i = 0; i < BOOK_PAGE_COUNT; ++i) {
        page_init(&(book->read_pages[i]));
    }
    page_init(&(book->write_page));
    book->write_position = 0;
    book->next_page = initial_page_number();
    book->root = strdup(root);
}

void book_close(book* book) {
    for (int i = 0; i < BOOK_PAGE_COUNT; ++i) {
        page_close(&(book->read_pages[i]));
    }
    page_close(&(book->write_page));
    free(book->root);
}

static page* book_get_page(book* book, uint32_t page_id) {
    int unused_page_index = -1;
    if (__builtin_expect(page_id == PAGE_NONE, false)) {
        return NULL;
    }
    if (book->write_page.page_id == page_id) {
        return &(book->write_page);
    }
    for (int i = 0; i < BOOK_PAGE_COUNT; ++i) {
        if (book->read_pages[i].page_id == page_id) {
            return &(book->read_pages[i]);
        }
        if (book->read_pages[i].page_id == PAGE_NONE) {
            unused_page_index = i;
        }
    }
    // Open a new page, evict another page at random
    page new_page;
    page_init(&new_page);
    if (page_open_read(book->root, &new_page, page_id)) {
        int evicted_page;
        if (unused_page_index == -1) {
            evicted_page = pick_random_page();
        } else {
            evicted_page = unused_page_index;
        }
        page_close(&(book->read_pages[evicted_page]));
        book->read_pages[evicted_page] = new_page;
        return &(book->read_pages[evicted_page]);
    } else {
        return NULL;
    }
}

size_t book_get(book* book, void* data, size_t maxlength, uint32_t page_id, uint32_t index) {
    if (index >= PAGE_SIZE) {
        return 0;
    }

    page* source_page = book_get_page(book, page_id);
    if (!source_page) {
        return 0;
    }

    const char* region_char = (const char*)source_page->region;

    if (index > 0) {
        // sanity check that this is preceded by a NUL
        if (region_char[index - 1] != '\0') {
            return 0;
        }
    }

    if (PAGE_SIZE - index < maxlength) {
        maxlength = PAGE_SIZE - index;
    }

    char* data_char = (char*)data;
    char* end_of_data_char = stpncpy(data_char, region_char + index, maxlength);
    return end_of_data_char - data_char;
}

bool book_put(book* book, const void* data, size_t length, uint32_t* page_id, uint32_t* index) {
    if (__builtin_expect(length > PAGE_SIZE - 1, false)) {
        return false;
    }

    if (
        book->write_page.page_id == PAGE_NONE ||
        (PAGE_SIZE - book->write_position) < (length + 1)
    ) {
        // Current page too small; close and open next one
        uint32_t this_page = book->next_page;
        uint32_t next_page = this_page;
        page_close(&(book->write_page));
        // Increment the next page _before_ the open; just in case this particular
        // page is difficult to open (e.g. permissions).
        ++next_page;
        if (next_page == 0 || next_page == PAGE_NONE) {
            // Wrap-around behaviour
            next_page = 1;
        }
        book->next_page = next_page;
        if (!page_open_write(book->root, &(book->write_page), this_page)) {
            return false;
        }
    }

    uint32_t offset = book->write_position;
    char* region_bytewise = (char*)(book->write_page.region);
    memcpy(region_bytewise + offset, data, length);
    region_bytewise[offset + length] = '\0';
    book->write_position = offset + length + 1;

    *page_id = book->write_page.page_id;
    *index = offset;
    return true;
}
