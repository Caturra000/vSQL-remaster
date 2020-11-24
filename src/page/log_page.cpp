#include "log_page.h"

void log_page::swap(log_page &that) {
    page::swap(that);
    std::swap(types, that.types);
    std::swap(timestamps, that.timestamps);
    std::swap(transactions, that.transactions);
    std::swap(pages, that.pages);
}

void log_page::add_log(uint32_t type, ts_t ts, tx_id_t tx_id, page_id_t page_id) {
    auto cur = get_cur_cnt();
    types[cur] = type;
    timestamps[cur] = ts;
    transactions[cur] = tx_id;
    pages[cur] = page_id;
    set_cur_cnt(cur+1);
}

void log_page::add_commit(ts_t ts, tx_id_t tx_id) {
    auto cur = get_cur_cnt();
    types[cur] = log_page::COMMITTED;
    timestamps[cur] = ts;
    transactions[cur] = tx_id;
    pages[cur] = page::ERROR_PAGE;
    set_cur_cnt(cur+1);
}

log_page::log_page(uint32_t magic): page(magic) {
    set_cur_cnt(0);
}

log_page::log_page(const log_page &that): page(that) { }

log_page::log_page(log_page &&that) noexcept:
        page(std::move(that)),
        types(that.types),
        timestamps(that.timestamps),
        transactions(that.transactions),
        pages(that.pages) {
    that.types = nullptr;
    that.timestamps = nullptr;
    that.transactions = nullptr;
    that.pages = nullptr;
}

log_page &log_page::operator=(log_page that) {
    this->swap(that);
    return *this;
}

log_page::log_type log_page::get_type(int pos) const {
    return reinterpret_cast<log_type*>(types)[pos];
}

page_id_t log_page::get_page_id(int pos) const {
    return reinterpret_cast<page_id_t*>(pages)[pos];
}

