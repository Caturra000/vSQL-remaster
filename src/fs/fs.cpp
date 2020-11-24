#include <iostream>
#include "fs.h"
#include "../utils/tools.h"

fs::fs(const std::string &file_name):
    file_name(file_name),
    fm(binder::get_binder().get_fm(file_name)),
    io(binder::get_binder().get_io(file_name)),
    rm(binder::get_binder().get_rm(file_name)),
    b_index(io, rm, header_factory::get_factory().get_header(file_name)) { }

fs::bplus_index &fs::get_index() {
    return b_index;
}

void fs::start_transaction() {
    auto& new_tx = io.new_tx_comes();
    b_index.set_tx(&new_tx);
}

// 不考虑性能
void fs::recover() {
    auto &tx_io = io.get_tx_io();
    auto &disk = io.get_disk();
    int log_cnt = tx_io.get_log_cnt();
    std::vector<std::pair<page_id_t,page>> redo_pages;
    std::vector<std::pair<page_id_t,page>> undo_pages;
    debug_only(
        std::cout << "log check.." << std::endl;
        std::cout << "log file count = " << log_cnt << std::endl;
        if(log_cnt == 0) std::cout << "it's safe to use now" << std::endl;
    );
    for(int i = 0; i < log_cnt; ++i) {
        // 获取log_page
        // 如果是commit，顺着走redo，清空v
        // 否则继续读入
        auto log = tx_io.read_log(i); // NRVO
        auto page_id = log.get_page_id(0);
        auto type = log.get_type(0);
        // 为了实现顺序写，此时单页日志只有一条简单的记录

        // 获取page_id
        switch(type) {
            case log_page::UNDO:
                undo_pages.emplace_back(page_id, tx_io.read_page(i));
                break;
            case log_page::REDO:
                redo_pages.emplace_back(page_id, tx_io.read_page(i));
                break;
            case log_page::COMMITTED:
                redo_pages.emplace_back(page_id, tx_io.read_page(i)); // 这里是头
                for(auto &it : redo_pages) {
                    auto &redo_page_id = it.first;
                    auto &redo_page = it.second;
                    disk.write(redo_page_id, redo_page);
                    debug_only(
                            std::cout << "redo complete" << std::endl;
                    );
                }
                disk.read(0, b_index.get_head());
                debug_only(
                        std::cout << "root = " << b_index.get_head().get_root() << std::endl;
                );
                b_index.set_root(b_index.get_head().get_root());
                redo_pages.clear();
                undo_pages.clear();
                break;
            default:
                std::cerr << "unsupported operation@recover()" << std::endl;
                break;
        }
    }
    if(!undo_pages.empty()) {
        // 由于各个页都是独立的，且事务单线程下肯定是连续的，走undo时不必倒着走
        for(auto &it : undo_pages) {
            auto &undo_page_id = it.first;
            auto &undo_page = it.second;
            disk.write(undo_page_id, undo_page);
        }
        debug_only(
            std::cout << "undo complete" << std::endl;
        );
    }
    debug_only(
        std::cout << "log operation done" << std::endl;
    );

}


void fs::commit() {
    // 把redo页都整过去
    // 把commit标记整过去
    // 下放放到已提交缓冲
    // 把index的tx置空
    if(!b_index.has_tx()) return;
    auto &cur_tx = b_index.get_tx();
    auto tx_id = cur_tx.get_tx_id();
    auto &redo_map = cur_tx.get_redo_map();
    auto &tx_io = io.get_tx_io();
    for(auto &it : redo_map) {
        auto &page_id = it.first;
        auto &page = *it.second;
        tx_io.append_log(page, log_page::REDO, 0, tx_id, page_id);
    }

    // 处理新增的record
    const auto &rec_page_ids = rm.get_rec_to_commit();
    debug_only(
        std::cout << "new record count to redo = " << rec_page_ids.size() << std::endl;
    );
    for(const auto &id : rec_page_ids) {
        const auto &page = io.read_op<rec_page>(id);
        tx_io.append_log(const_cast<rec_page&>(page), log_page::REDO, 0, tx_id, id);
    }
    rm.flush_rec_to_commit();
    auto &hp = b_index.get_head();
    tx_io.append_log(hp, log_page::COMMITTED, 0, tx_id, 0, true);

    for(auto &it : redo_map) {
        auto &page_id = it.first;
        auto &page_ptr = it.second;
        io.replace_page_specific(page_id, page_ptr); // 转移page的资源
    }
    //std::cout << "transaction " << tx_id << " commited!" << std::endl;
    b_index.set_tx(nullptr);
    io.get_tm().remove_tx(tx_id); // 已经不会再用到了

}

