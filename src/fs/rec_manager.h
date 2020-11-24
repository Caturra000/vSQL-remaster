#ifndef VSQL_REC_MANAGER_H
#define VSQL_REC_MANAGER_H

#include "../page/head_page.h"
#include "../page/rec_page.h"
#include "free_manager.h"
#include "io_manager.h"
#include <unordered_set>

// 行记录（record）管理
class rec_manager {
    io_manager &io;
    head_page &head;
public:
//    rec_manager(const std::string &file_name, free_manager &fm)
//            : _io(file_name, fm),
//              head(header_factory::get_factory().get_header(file_name)) { }
//
//    rec_manager(const std::string &file_name, free_manager &fm, head_page &head)
//            : _io(file_name, fm), head(head) { }

    rec_manager(io_manager &io, head_page &head): io(io), head(head) {}
    rec_manager(): io(io_manager::null()), head(header_factory::get_factory().get_header("null")) {}

    // 如果一个记录太大，那还能怎么搞？
    // 类似一个buddy/slab的算法？
    // 如果刷新旧的刚好是rec类型，就把一个备胎池刷出？
    // 或者从简，留下空洞文件，以后再整理？   ====> 这个方便点

    // 这个类用于维护最新的一份记录页，不如从head进行改造一波？

    std::unordered_set<page_id_t> rec_to_commit; // 记录不需要UNDO(由于LAZY机制,索引保证能找到正确的记录), 只需redo

    // 如果塞得下，do，返回当前下标
    // 否则发一个新的页，并重复刚才的事情
    // 默认记录肯定不超过PAGE_SIZE（不打算写溢出页了，没意思）
    page_off_t add_rec(void *rec, size_t n_byte) {
        auto rec_id = head.get_last_rec(); // 从头部获得
        if(rec_id == page::ERROR_PAGE || !io.read_op<rec_page>(rec_id).can_store(n_byte)) {
            rec_id = io.alloc<rec_page>(rec_page::RECORD);
            head.set_last_rec(rec_id);
        }
        rec_to_commit.insert(rec_id); // 用于事务优化
        auto rec_offset = io.read_op<rec_page>(rec_id).get_ptr_cnt();
        io.write_op<rec_page>(rec_id).add_rec(rec, n_byte);

        return ((page_off_t)rec_offset << 32) | rec_id;
    }

    const std::unordered_set<page_id_t>& get_rec_to_commit() {
        return rec_to_commit;
    }

    void flush_rec_to_commit() {
        rec_to_commit.clear();
    }


    size_t get_rec(page_off_t pof, const void *&rec) {
        page_id_t pid = pof; // 截断
        offset_t  off = pof >> 32;
        rec = io.read_op<rec_page>(pid).get_rec(off);
        return io.read_op<rec_page>(pid).get_rec_size(off);
    }

};

#endif //VSQL_REC_MANAGER_H
