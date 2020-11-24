#ifndef VSQL_TX_IO_H
#define VSQL_TX_IO_H

#include <cstdio>
#include <string>
#include <unordered_map>
#include "../page/head_page.h"
#include "../page/log_page.h"

class tx_io {
    FILE *file;  // 日志关键信息
    FILE *h_file; // 头 // TODO 删去
    std::string file_name;
    std::string file_name_log;
    std::unordered_map<uint32_t, log_page> log_cache;
    uint32_t log_cnt;
public:
    explicit tx_io(const std::string &file_name);
    void append_log(page &page, uint32_t type, ts_t ts, tx_id_t tx_id, page_id_t page_id, bool force_flush = false);
    // log_page& get_log(uint32_t log_id); // 单线程写下不用缓存了 因为没有冲突
    uint32_t get_log_cnt() const;
    void add_log_cnt();
    log_page read_log(uint32_t log_id);
    page read_page(uint32_t log_id);
    ~tx_io();
private:
    // 方便使用
    static FILE* open(const std::string &path);
};

#endif //VSQL_TX_IO_H
