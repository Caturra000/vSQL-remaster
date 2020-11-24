#ifndef VSQL_FREE_MANAGER_H
#define VSQL_FREE_MANAGER_H

#include <stack>
#include <iostream>
#include "../page/page.h"
#include "../page/head_page.h"
#include "disk.h"
#include "header_factory.h"

// 用于页的回收复用
// 但自从索引提供懒惰删除策略之后，基本上用不着了
class free_manager {
    std::stack<page_id_t> free_pages;
    head_page &head;
    disk disk;
public:
    // 一个非常低成本的转移方案，仅需拷贝几个字节而非8K就能完成所有分配
    template <typename page_type>
    page_id_t get_as(page_type &swapped);

    void add_to_free(page_id_t page_id, page &page);
    free_manager(const std::string &file_name, head_page &head); // 已经得知head_page，不需要额外的mapping
    explicit free_manager(const std::string &file_name);
    ~free_manager();

    // 仅用于空标记
    free_manager();
    static free_manager& null_obj();
};

template<typename page_type>
page_id_t free_manager::get_as(page_type &swapped) {
    static page_type temp(page::FAST_COPY); // 先删了这个特性
    if(!free_pages.empty()) {
        auto page_id = free_pages.top();
        free_pages.pop();
        auto first_free = free_pages.empty() ?
                          page::ERROR_PAGE : free_pages.top();
        head.set_first_free(first_free);
        swapped = temp; // 特殊的FAST_COPY拷贝，只会对头部赋值 // TODO 改成更直观的形式
        return page_id;
    } else {
        auto cur_id = head.get_page_cnt();
        head.set_page_cnt(cur_id+1);
        swapped = temp;
        return cur_id;
    }
}


#endif //VSQL_FREE_MANAGER_H
