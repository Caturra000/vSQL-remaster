#ifndef VSQL_IO_MANAGER_H
#define VSQL_IO_MANAGER_H



// IO块仅针对一个表的IO维护，采用一表一文件的形式，其中0页表示表信息（只能通过固定的形式访问），也表示错误的寻址
// 问题是两种page对应了同一套page id === > 解决，分两套实现
// TODO 感觉区分读LRU和写LRU会更加好，因为可能存在过旧的脏页和崭新但却是不脏的页，此时交换应该是不脏的页，这样能减缓一次IO过程（极端情况是2次）
// 分配新的页不一定要先写入磁盘，可以从淘汰的页中直接内存级别的替换，只要修改mapping就好


#include <vector>
#include <bitset>
#include <memory>
#include <set>
#include "../page/page.h"
#include "free_manager.h"
#include "lru_cache.h"
#include "../utils/duplex_map.h"
#include "../tx/tx_manager.h"

class io_manager {
    const static size_t CACHE_CAPACITY = 8192;
    const static size_t ERROR_INDEX = CACHE_CAPACITY;

    std::bitset<CACHE_CAPACITY> dirty;
    lru_cache<page_id_t> cache; // 只维护lru中的头和尾，不存放具体数据
    // std::vector<page_type> pages;
    // ↑私有缓存 废弃 通过干掉io_manager的template和智能指针的多态来实现共享,但由于每个页类型其实差异非常大,只提供简单的读写算了
    // 如果希望更加通用的做法,就是为page引入virtual的读和写来匹配io,
    // 不过这也没啥卵用,因为算法实现上必须知道每个page_id对应哪个类型,都知道类型了那就直接template函数吧

    // 表内共享缓存 打算设计为已提交读缓冲区
    std::vector<std::unique_ptr<page>> pages; // 由于使用unique，在对外接口看来外部只能获取极其短暂的（作用域内）引用

    disk hard_disk;
    duplex_map<page_id_t, size_t, 1 << 20, CACHE_CAPACITY> map;
    tx_manager &tm;
    free_manager &fm;


    //tx_manager tx_manager; // 该表当前事务的集合，以及日志持久化工具
    //std::map<page_id_t, std::set<tx_id_t>> watchers; // 用于观察者模式，性能上可能捉急一点



    template <typename page_type>
    page_type& common_op(page_id_t page_id) {
        static_assert(std::is_base_of<page, page_type>::value, "must be a page");
        auto index = map.find(page_id);
        if(index != ERROR_INDEX) {
            cache.access(index);
            return static_cast<page_type&>(*pages[index]);
        }
        auto swapped = cache.least();
        // 淘汰，脏则刷入磁盘 移除映射
        replace(swapped);

        // 置换
        if(!pages[swapped]) {
            pages[swapped].reset(new page_type());
        }
        hard_disk.read(page_id, *pages[swapped]);
        // 添加映射
        map.create_mapping(page_id, swapped);
        cache.access(swapped);
        return static_cast<page_type&>(*pages[swapped]);
    }


public:
    io_manager():
            cache(222), pages(222), hard_disk("null"),
            map(page::ERROR_PAGE,ERROR_INDEX), fm(free_manager::null_obj()),
            tm(tx_manager::null()) { }

    io_manager(const std::string &file_name, free_manager &fm, tx_manager &tm):
            cache(CACHE_CAPACITY), pages(CACHE_CAPACITY),
            hard_disk(file_name), map(page::ERROR_PAGE, ERROR_INDEX), // page->index
            fm(fm), tm(tm) { }

    template <typename page_type>
    const page_type& read_op(page_id_t page_id) {
        return common_op<page_type>(page_id);
    }

    template <typename page_type>
    page_type& write_op(page_id_t page_id) {
        auto &page = common_op<page_type>(page_id);
        dirty.set(map.find(page_id));
        return page;
    }

    void replace(size_t index) {
        if(dirty[index]) {
            hard_disk.write(map.find_rev(index), *pages[index]);
            dirty[index] = false;
            // delete pages[index];
            pages[index].reset();
        }
        map.erase_mapping_rev(index);
    }

    template<typename page_type>
    page_id_t alloc(uint32_t magic = page::UNDEFINED) {
        auto swapped = cache.least();
        replace(swapped);
        if(pages[swapped] == nullptr) {
            pages[swapped].reset(new page_type());
        }
        auto page_id = fm.get_as<page_type>(static_cast<page_type&>(*pages[swapped]));
        map.create_mapping(page_id, swapped); // page_id => index
        cache.access(swapped);
        pages[swapped]->set_magic(magic);
        dirty[swapped] = true;
        return page_id;
    }

    // LAZY下不会用到了
    template <typename page_type>
    void dealloc(page_id_t page_id, bool first_time = true) {
        auto index = map.find(page_id);
        if(index == ERROR_INDEX && first_time) { // 在磁盘
            read_op<page_type>(page_id); // 默认是在磁盘里，刷入缓存
            dealloc<page_type>(page_id,false);
            return;
        }
        if(index == ERROR_INDEX) return; // 确实不在磁盘
        // 在缓存内部
        fm.add_to_free(page_id, *pages[index]);
        map.erase_mapping_rev(index);
        dirty[index] = false; // 交给fm处理持久化
    }


    ~io_manager() {
        for(size_t i = 0; i < CACHE_CAPACITY; i++) {
            auto page_id = map.find_rev(i);
            if(page_id == page::ERROR_PAGE) continue;
            if(dirty[i]) hard_disk.write(page_id, *pages[i]);
        }
    }

    static io_manager& null() {
        static io_manager null_obj;
        return null_obj;
    }

    tx& new_tx_comes() {
        auto tx_id  = tm.next_tx_id();
        return tm.get_tx(tx_id); // 不存在时生成新的tx
    }

    // 事务相关的装饰器函数

    // 虽然很想细分为read_op时采用copy on write，当不存在时只读共享缓冲pages的数据
    // 但是感觉很麻烦也对不上common_op这个接口（必须经过，不区分r/w），想想还是算了，但这确实是一个可优化的点

    template <typename page_type>
    page_type& common_op(tx& cur_tx, page_id_t page_id) {
        if(!cur_tx.has_page(page_id)) {
            auto& page = common_op<page_type>(page_id);
            cur_tx.add_page<page_type>(page_id, page);
        }
        return cur_tx.get_page<page_type>(page_id);
    }

//    template <typename page_type>
//    page_id_t alloc(tx &cur_tx, uint32_t magic = page::UNDEFINED) {
//        return alloc<page_type>(magic); // 一个方便的做法
////        auto& page = common_op<page_type>(page_id);
////        cur_tx.add_page<page_type>(page_id, page); // 还是把不存在这种粗活交给common_op吧
////        return page_id; // 无论版本是私有还是共享，页id总是保持一致的
//    }

    template <typename page_type>
    const page_type& read_op(tx &cur_tx, page_id_t page_id) {
        return common_op<page_type>(cur_tx, page_id);
    }

    template <typename page_type>
    page_type& write_op(tx &cur_tx, page_id_t page_id) {
        if(!cur_tx.has_backup(page_id)) {
            //common_op<page_type>(cur_tx, page_id); ，先备份一个（副作用暴力实现）

            if(!cur_tx.has_page(page_id)) { // 可能根本就没有
                auto& page = common_op<page_type>(page_id);
                cur_tx.add_page<page_type>(page_id, page);
            }
            auto &p = cur_tx.backup<page_type>(page_id);
            auto &tx_io = tm.get_tx_io();
            tx_io.append_log(p, log_page::UNDO, 0, cur_tx.get_tx_id(), page_id, true); // 强制写入 TODO 可能commit时再force比较好？
        }
        return common_op<page_type>(cur_tx, page_id);
    }

    tx_io& get_tx_io() {
        return tm.get_tx_io();
    }

    disk& get_disk() {
        return hard_disk;
    }

    // p的多态由tx内部的指针指向不截断保证
    // 注意这个会破坏按照key-value的访问形式
    // 只适用于销毁前
    void replace_page_specific(page_id_t page_id, std::unique_ptr<page> &page_ptr) {
        common_op<page>(page_id); // 偷懒一下
        auto index = map.find(page_id);
        pages[index] = std::move(page_ptr); // 当前事务的状态更新到已提交缓冲中
        dirty[index] = true;
    }

    tx_manager& get_tm() {
        return tm;
    }



};




#endif //VSQL_IO_MANAGER_H

