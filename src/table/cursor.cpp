#include "cursor.h"
#include "parser.h"
#include "../utils/guard.h"

table& cursor::get_table() {
    return belong_to;
}

cursor::cursor(table &table) : belong_to(table) { }

iterator &cursor::get_iter() {
    return iter;
}

// 构造完查询约束后 更新迭代器内部的结果， 并且rewind
// PS.我觉得table文件夹这一块的函数写的特别零散，又没啥好方法
void cursor::query() {
    using index_page = bplus_page<pri_key_t>;
    iter.clear();
    if(!iter.idx_stat().runnable()) return;
    auto &index = belong_to.get_fs().get_index();
    auto root = index.get_root();
    if(root == page::ERROR_PAGE) return;
    auto &status = iter.idx_stat();
    auto lb = status.get_lo() + (!status.lo_open() ? 0 : 1);
    // 目前固定是1，后期可加入浮点类型
    // std::numeric_limits<pri_key_t>::is_integer ? 1 : std::numeric_limits<pri_key_t>::min()
    auto leaf = index.down_to_leaf(lb);
    if(leaf == page::ERROR_PAGE) return;
    auto &bio = index.get_bio();
    auto pos = bio.read_op<index_page>(leaf).find_first_ge(lb);
    auto ub = status.get_hi() - (!status.hi_open() ? 0 : 1);

    auto parse = parser(belong_to);
    while(true) {
        if(pos >= bio.read_op<index_page>(leaf).get_key_cnt()) {
            auto next = bio.read_op<index_page>(leaf).get_next();
            if(next == page::ERROR_PAGE) return;
            pos = 0;
            leaf = next;
        } else {
            if(bio.read_op<index_page>(leaf).get_key(pos) > ub) return; // 取代valid检查
            auto ch = bio.read_op<index_page>(leaf).get_ch(pos);
            auto is_lazy = (ch == page::ERROR_PAGE);
            if(!is_lazy) {
                const void *raw_rec = nullptr;
                auto rec_size = index.output(ch, raw_rec); // 不会产生内存泄漏
                iter.add_rec(parse.raw2rec(reinterpret_cast<const char*>(raw_rec), rec_size));
            }
            ++pos;
        }
    }
}


// 下面三个不检查
//
//void cursor::add() {
//    auto &index = belong_to.get_fs().get_index();
//    for(iter.begin(); !iter.eof(); iter.next()) {
//        char *raw = nullptr;
//        auto g = guard::make([&](){ delete[] raw; });
//        auto &rec = iter.cur_record();
//        auto key = rec.get_rowid();
//        auto n_byte = parser(belong_to).rec2raw(rec,raw);
//        index.add(key, raw, n_byte);
//    }
//}
//
//void cursor::remove() {
//    auto &index = belong_to.get_fs().get_index();
//    std::vector<page_off_t> pof;
//    for(iter.begin(); !iter.eof(); iter.next()) {
//        auto &rec = iter.cur_record();
//        auto key = rec.get_rowid();
//        index.remove(key, pof);
//    }
//}
//
//void cursor::update() {
//    remove(); // 只用key，简单粗暴
//    add();
//}

void cursor::rewind() {
    iter.begin();
}

void cursor::next() {
    iter.next();
}

bool cursor::eof() {
    return iter.eof();
}

