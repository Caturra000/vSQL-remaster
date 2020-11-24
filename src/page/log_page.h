#ifndef VSQL_LOG_PAGE_H
#define VSQL_LOG_PAGE_H

#include "page.h"

class log_page : public page {
protected:
    void swap(log_page &that);
public:

    // TODO: 日志总计个数放入head中，现在默认单页足够使用

    accessor(cur_cnt, size_t, field_end_magic); // 当前日志个数
    constexpr static size_t fixed_end = field_end_cur_cnt;
    constexpr static size_t unit_size = sizeof(uint32_t) + sizeof(ts_t) + sizeof(tx_id_t) + sizeof(page_id_t);
    constexpr static size_t unit_count = (PAGE_SIZE - fixed_end) / unit_size;
private:
    partition(types, uint32_t, fixed_end, field_start_types + unit_count * unit_size_types);
    partition(timestamps, ts_t, field_end_types, field_start_timestamps + unit_count * unit_size_timestamps);
    partition(transactions, tx_id_t, field_end_timestamps, field_start_transactions + unit_count * unit_size_transactions);
    partition(pages, page_id_t, field_end_transactions, field_start_pages + unit_count * unit_size_pages);
public:
    void add_log(uint32_t type, ts_t ts, tx_id_t tx_id, page_id_t page_id);
    void add_commit(ts_t ts, tx_id_t tx_id);


    // void add_abort() { } // 暂不实现

    enum magic_type { LOG = 0x4444 };

    enum log_type {
        COMMITTED,
        ABORTED,
        REDO,
        UNDO,
    };

    log_type get_type(int pos) const;
    page_id_t get_page_id(int pos) const;


    explicit log_page(uint32_t magic = log_page::LOG);
    log_page(const log_page &that);
    log_page(log_page &&that) noexcept;
    log_page& operator = (log_page that);



};

#endif //VSQL_LOG_PAGE_H
