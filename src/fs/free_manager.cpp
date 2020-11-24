#include <iostream>
#include "../defs/defs.h"
#include "free_manager.h"



void free_manager::add_to_free(page_id_t page_id, page &page) {
    auto first = free_pages.empty() ?
                 page::ERROR_PAGE : free_pages.top();
    page.set_magic(page::FREE);
    page.set_next_free(first);
    head.set_first_free(page_id); // 成为first
    free_pages.push(page_id); // 内存形式
}

free_manager::free_manager(const std::string &file_name, head_page &head)
        : head(head), disk(file_name) {
    auto next = head.get_first_free();
    std::stack<page_id_t> fifo;
    page temp;
    while (next != page::ERROR_PAGE) {
        fifo.push(next);
        auto page_id = next;
        disk.read(page_id, temp);
        next = temp.get_next_free();
    }
    while (!fifo.empty()) { // 构造(栈顶)[first(free0)->free1->2->3->0),不包含0
        free_pages.push(fifo.top());
        fifo.pop();
    }
}

free_manager::free_manager(const std::string &file_name)
        : free_manager(file_name, header_factory
::get_factory().get_header(file_name)) { }

free_manager::~free_manager() {
    std::cout << "fm delete" << std::endl;
    page tmp_page(page::FREE);
    while(!free_pages.empty()) {
        auto page_id = free_pages.top();
        free_pages.pop();
        tmp_page.set_next_free(free_pages.empty() ?
                               page::ERROR_PAGE : free_pages.top());   // FIXED 修复外存错误
        disk.write(page_id,tmp_page);
    }
}

//free_manager& free_manager::null_free_manager() {
//    static free_manager null("null");
//    return null;
//}

free_manager::free_manager(): disk("null"), head(header_factory::get_factory().get_header("null")) {  }

free_manager &free_manager::null_obj() {
    static free_manager null;
    return null;
}

