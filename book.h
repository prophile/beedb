#ifndef __INCLUDED_BOOK_H
#define __INCLUDED_BOOK_H

#include "page.h"

const static int BOOK_PAGE_COUNT = 32;

typedef struct _book {
    page read_pages[BOOK_PAGE_COUNT];
    page write_page;
    uint32_t write_position;
    uint32_t next_page;
    char* root;
} book;

void book_init(book* book, const char* root);
void book_close(book* book);

size_t book_get(book* book, void* data, size_t maxlength, uint32_t page_id, uint32_t index);
bool book_put(book* book, const void* data, size_t length, uint32_t* page_id, uint32_t* index);

#endif
