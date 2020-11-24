#include "tx_io.h"
#include "../page/log_page.h"

tx_io::tx_io(const std::string &file_name):
        file_name(file_name), file_name_log(file_name+".log"), file(open(file_name+"log")), h_file(open(file_name)) {
    fseek(file, 0, SEEK_END);
    auto file_size = ftell(file); // TODO 没有验证tell到底是最后一个还是只是哨兵
    fseek(file, 0, SEEK_SET);
    log_cnt = file_size / (PAGE_SIZE * 2);
}

void tx_io::append_log(page &page, uint32_t type, ts_t ts, tx_id_t tx_id, page_id_t page_id, bool force_flush) {
    auto log_id = get_log_cnt();
    log_page lp; // 这里是可改进的地方  把add换成set lp 变成static。。。
    lp.add_log(type, ts, tx_id, page_id);
    fseek(file, 1ull * log_id * PAGE_SIZE * 2, SEEK_SET);
    fwrite(lp.get_data(), PAGE_SIZE, 1, file);
    fseek(file, 1ull * log_id * PAGE_SIZE * 2 + PAGE_SIZE, SEEK_SET); // 这样可保持大量的日志写入也是顺序写，总是按照[log][page]...这种形式
    fwrite(page.get_data(), PAGE_SIZE, 1, file);
    if(force_flush) fflush(file);
    add_log_cnt();
}

FILE* tx_io::open(const std::string &path) {
    FILE *f = fopen(path.c_str(), "rb+");
    if(!f) {
        f = fopen(path.c_str(), "wb");
        fclose(f);
        f = fopen(path.c_str(), "rb+");
    }
    return f;
}

//log_page &tx_io::get_log(uint32_t log_id) {
//    if(log_cache.find(log_id) == log_cache.end()) {
//        log_page temp;
//        // open....TODO
//        // fwrite...
//        log_cache.emplace(log_id, std::move(temp));
//    }
//    return log_cache[log_id];
//}

tx_io::~tx_io() {
    fclose(file);
    fclose(h_file);
}

uint32_t tx_io::get_log_cnt() const {
    return log_cnt;
}

void tx_io::add_log_cnt() {
    ++log_cnt;
}

log_page tx_io::read_log(uint32_t log_id) {
    log_page lp;
    fseek(file, 1ull *log_id * PAGE_SIZE * 2, SEEK_SET);
    lp.overlap_from(file);
    return lp;
}

page tx_io::read_page(uint32_t log_id) {
    page p;
    fseek(file, 1ull * log_id * PAGE_SIZE * 2 + PAGE_SIZE, SEEK_SET);
    p.overlap_from(file);
    return p;
}


