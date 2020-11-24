#include "page.h"

// page::page() : data(new char[PAGE_SIZE]) { set_magic(UNDEFINED); }

page::page(uint32_t magic):
        data(new char[PAGE_SIZE]) { set_magic(magic); }

page::page(const page &that): page() {
    memcpy(this->data, that.data, PAGE_SIZE);
}

page::page(char *&&temp_data): data(temp_data) { temp_data = nullptr; }

page::~page() { delete[] data; }

//page &page::operator=(const page &that) {
//    if(this == &that) return *this;
//    memcpy(data, that.data, PAGE_SIZE);
//    return *this;
//}

bool page::is_free() const {
    return get_magic() == FREE;
}

bool page::is_fast_copy() const {
    return get_magic() == FAST_COPY;
}

page_id_t page::get_next_free() const {
    if(is_free()) {
        return *reinterpret_cast<page_id_t*>(data + fixed_end_if_free);
    }
    return ERROR_PAGE;
}

void page::set_next_free(page_id_t next_free) {
    if(is_free()) {
        *reinterpret_cast<page_id_t*>(data + fixed_end_if_free) = next_free;
    }
}

const char *page::get_data() const {
    return data;
}

void page::swap(page &that) {
    std::swap(data, that.data);
}

page::page(page &&that) noexcept: data(that.data) {
    that.data = nullptr;
}

page &page::operator=(page that) {
    this->swap(that);
    return *this;
}

void page::overlap_from(FILE *file) {
    fread(data, PAGE_SIZE, 1, file);
}



