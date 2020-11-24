#include "head_page.h"

head_page::head_page(uint32_t magic) : page(magic) {
    set_page_cnt(1);
    set_root(page::ERROR_PAGE);
    set_first_free(page::ERROR_PAGE);
    set_last_rec(page::ERROR_PAGE);
    table_info[0] = 0;
}

head_page::head_page(char *&&temp_data) : page(std::move(temp_data)) {  }

head_page::head_page(const head_page &that): page(that) { }

head_page::head_page(head_page &&that) noexcept:
        page(std::move(that)), table_info(that.table_info) {
    that.table_info = nullptr;
}

void head_page::swap(head_page &that) {
    page::swap(that);
    std::swap(table_info, that.table_info);
}

head_page &head_page::operator=(head_page that) {
    this->swap(that);
    return *this;
}
