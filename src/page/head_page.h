#ifndef VSQL_HEAD_PAGE_H
#define VSQL_HEAD_PAGE_H


#include "page.h"

class head_page : public page {
protected:
    void swap(head_page &that);
public:
    explicit head_page(uint32_t magic = HEAD);
    explicit head_page(char *&&temp_data);
    head_page(const head_page &that);
    head_page(head_page &&that) noexcept;
    head_page& operator = (head_page that);

    accessor(page_cnt, uint32_t, field_end_magic);
    accessor(root, page_id_t, field_end_page_cnt);
    accessor(last_rec, page_id_t, field_end_root); // 最后一个已分配的未满记录页， 允许空洞文件存在
    accessor(first_free, uint32_t, field_end_last_rec);
    accessor(log_cnt, uint32_t, field_end_first_free);
private:
    partition(table_info, char, field_end_log_cnt, PAGE_SIZE); // 文本形式， 空格分割  \0 结束
    // 按照SQLITE输入顺序直接move进去就好了，这样形式统一，解析起来不累
    // 主要就三块， name column key
public:
    enum magic_type {
        HEAD = 0xCAFF,
    };

};



#endif //VSQL_HEAD_PAGE_H
