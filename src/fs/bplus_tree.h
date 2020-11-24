#ifndef VSQL_BPLUS_TREE_H
#define VSQL_BPLUS_TREE_H

#include "../page/mem_node.h"
#include "rec_manager.h"
#include "fast_io.h"
#include "index.h"

// 关于减少事务的侵入性：
// 目前只使用了比较纯粹的tx*和io&进行简单配合
// 每个操作在获得page都经过带tx参数的io接口，这样无论是写操作还是读操作，对于b+树都是看到事物内部的私有状态

// 而像find down_to_leaf这种只有读（非事务）的操作则直接从io的全局buffer中获取

// 关于log文件的记录还是使用被动的形式吧（懒）
// 因为修改已经在事务内部的after中记下了，所以不需要额外写什么log
// 而before是只要有【一定倾向】就要首先刷入盘
//   到底是一旦存在就立刻写盘
//   还是一直盯着事务状态？这种情况可以把before给省略掉，完成的一瞬间把redo的所有日志都写到磁盘
//   具体一点是，盯着，刷出时先放日志，引用为0则commit........好像这样太麻烦了 没法保证后面覆盖上来但是崩掉的原子性
//    还是按照最简单的方法来做吧，redo undo都写， undo第一次立刻写入盘， redo我开心就写 这样肯定没错

// 目前bplus_tree的主键固定使用int64_t
// 后期需要消除template达到运行时确认再使用index
// 接口对于record类型的封装交给table或者fs管理
// 模板带入comp
template<typename key_t>
class bplus_tree /* : public index */ {
    using index_page = bplus_page<key_t>;
protected:
    page_id_t      root;
    io_manager     &bio;   // 用于结点更新和分配 注意fast_io只能表内各类型全局一份
    rec_manager    &rm;    // 用于记录的更新
    head_page      &head; // 用于根的更新
    // 当开启新事务时修改tx指向 （方便sqlite接口）
    tx             *cur_tx { nullptr }; // 默认作为查询无需事务状态
public:
//    bplus_tree(const std::string &file_name, free_manager &fm, head_page &head);
//
//    bplus_tree(const std::string &file_name, free_manager &fm);

    bplus_tree(io_manager &bio, rec_manager &rm, head_page &head);


    // 返回的是对应的叶子节点id
    page_id_t find_range(key_t lb, key_t ub,
                         std::vector<page_off_t> &records,
                         int need = std::numeric_limits<int>::max());

    page_id_t find(key_t key, std::vector<page_off_t> &record);

    void add(key_t key, void *record, size_t n_byte);

    // LAZY remove
    void remove(key_t key, std::vector<page_off_t> &record);

    void remove(key_t key);

    //  添加了LAZY删除策略后，可以大幅度加强批量删除的性能
    // void remove_range

    size_t output(page_off_t pof, const void *&rec);
    io_manager& get_bio();


    // 针对迭代器的接口，功能与直接使用find类似，但支持约束检查
//    void query(iterator& iter);

    // 旧的API，被LAZY remove取代
    // void remove_nolazy(key_t key, std::vector<page_off_t> &record);


    // FIX 目前考虑删除操作的LAZY处理，以进一步提高性能 ===> 完成了，在原接口上只需修改两三处
    // 我们认为一个记录不存在，有两种可能
    // 1. 要么根本不存在对应的叶子位置
    // 2. 叶子对应记录位置的child为ERROR_PAGE
    // 这样可以在回收时只需接触索引页而不必寻址到记录页，减少IO
    // 浪费的记录页不做处理，避免分页槽的移动操作   还有避免了for i ch update parent 这种疯狂的写操作
    // 只有重启表时才针对这些文件进行重新构造，以获得更好的顺序读写（比如把所有record排个序再直接分配，目前先不管）
    // ↑这个好像可以做成一个可达性分析的GC装置

public:

    // 解决两个麻烦：
    // 简单的查询不需要事务（但有些查询又会在事务内），减少在树操作中对事务对象的依赖
    // 解决指针到引用的鬼畜转换

    template<typename page_type>
    const page_type& read_op(page_id_t page_id) {
        if(cur_tx) return bio.read_op<page_type>(*cur_tx, page_id);
        return bio.read_op<page_type>(page_id);
    }
    
    template<typename page_type>
    page_type& write_op(page_id_t page_id) {
        if(cur_tx) return bio.write_op<page_type>(*cur_tx, page_id);
        return bio.write_op<page_type>(page_id);
    }
    
    // ok
    page_id_t down_to_leaf(key_t key);



    // ok
    void traverse(page_id_t leaf_id,
                  page_off_t pos,
                  key_t ub,
                  std::vector<page_off_t> &records,
                  int need = std::numeric_limits<int>::max());

    // ok
    void insert_parent(page_id_t l_node_id,
                       key_t key,
                       page_id_t r_node_id);

    // ok
    key_t split(page_id_t full_node_id,
                page_id_t new_node_id,
                key_t key,
                page_off_t rec);


    // OK
    key_t split(page_id_t full_node_id,
                page_id_t new_node_id,
                key_t key,
                page_id_t l_ch_id,
                page_id_t r_ch_id);


    // OK
    void remove(page_id_t node_id, key_t key, page_id_t ch_id);

    // . 感觉可以对频繁写入作出进一步优化
    // 针对不在缓存中的页，只要不是读取记录，其实可以把写的命令暂存起来，只有当真正要读的时候在LRU置换中获取并更新这些写命令
    // 这样可以针对 for(i : node->children) i->set_parent(node) 这种潜在的大量刷盘又没啥卵用的写操作进行缓解
    // 可是这样又要实现一个类似op code的大轮子，不值得， 以后再做

    // OK
    void coalesce_condition(page_id_t node, page_id_t neighbor,
                            int64_t neighbor_pos, key_t mid_k);

    // OK
    void remove_root_condition();


    // OK
    void overflow_condition(page_id_t node, page_id_t neighbor,
                            int neighbor_pos, int key_pos, key_t key);

    page_id_t get_root();

    void set_tx(tx *new_tx);
    bool has_tx();
    tx& get_tx();

    head_page &get_head();
    void set_root(page_id_t new_root);
};

//template<typename key_t>
//bplus_tree<key_t>::bplus_tree(const std::string &file_name, free_manager &fm, head_page &head)
//        : bio(file_name,fm), rm(file_name,fm,head), head(head) {
//    root = head.get_root();
//}
//
//template<typename key_t>
//bplus_tree<key_t>::bplus_tree(const std::string &file_name, free_manager &fm)
//        : bplus_tree(file_name, fm,
//                     header_factory::get_factory().get_header(file_name)) {
//    root = head.get_root();
//}

template<typename key_t>
page_id_t bplus_tree<key_t>::find_range(key_t lb, key_t ub, std::vector<page_off_t> &records, int need) {
    auto leaf_id = down_to_leaf(lb);
    if(leaf_id == page::ERROR_PAGE) return leaf_id;
    auto pos = read_op<index_page>(leaf_id).find_first_ge(lb);
    traverse(leaf_id, pos, ub, records, need);
    return leaf_id;
}

template<typename key_t>
page_id_t bplus_tree<key_t>::find(key_t key, std::vector<page_off_t> &record) {
    return find_range(key, key, record, 1);
}


template<typename key_t>
void bplus_tree<key_t>::add(key_t key, void *record, size_t n_byte) {
    auto leaf_id = down_to_leaf(key);
    if(leaf_id == page::ERROR_PAGE) {
        leaf_id = bio.alloc<index_page>(bplus_page<key_t>::ROOT | bplus_page<key_t>::LEAF);
        head.set_root(root = leaf_id);
    }
    auto rec_pos = rm.add_rec(record, n_byte);
    const auto &leaf_node = read_op<index_page>(leaf_id);
    auto pos = leaf_node.find_key(key);
    bool is_lazy = (pos != leaf_node.get_key_cnt()
                    && leaf_node.get_ch(pos) == page::ERROR_PAGE);
    bool is_duplicate = (pos != leaf_node.get_key_cnt()
                    && leaf_node.get_ch(pos) != page::ERROR_PAGE);
    if(is_lazy) {
        write_op<index_page>(leaf_id).set_ch(pos, rec_pos);
    } else if(is_duplicate) {
        return;
    } else if(!leaf_node.is_full()) {
        write_op<index_page>(leaf_id).insert_notfull(key, rec_pos);
    } else {
        auto new_leaf = bio.alloc<index_page>(bplus_page<key_t>::LEAF);
        auto new_key = split(leaf_id, new_leaf, key, rec_pos);
        insert_parent(leaf_id, new_key, new_leaf);
    }
}

template<typename key_t>
void bplus_tree<key_t>::remove(key_t key, std::vector<page_off_t> &record) {
    auto leaf_id = down_to_leaf(key);
    if(leaf_id == page::ERROR_PAGE) return;
    const auto &leaf = read_op<index_page>(leaf_id);
    auto pos = leaf.find_key(key);
    if(pos == leaf.get_key_cnt()) return; // 确实不存在
    auto rec_pos = leaf.get_ch(pos);
    if(rec_pos == page::ERROR_PAGE) return; // 早已打上了LAZY标记
    record.push_back(rec_pos);
    write_op<index_page>(leaf_id).set_ch(pos, page::ERROR_PAGE); // LAZY delete
}

template<typename key_t>
size_t bplus_tree<key_t>::output(page_off_t pof, const void *&rec) {
    return rm.get_rec(pof,rec);
}

//template<typename key_t>
//void bplus_tree<key_t>::remove_nolazy(key_t key, std::vector<page_off_t> &record) {
//    auto leaf = find(key, record);
//    if(leaf == page::ERROR_PAGE) return;
//    if(record.empty()) return;
//    auto pos = read_op<index_page>(leaf).find_first_ge(key);
//    remove(leaf, key, read_op<index_page>(leaf).get_ch(pos));
//    // 这里可手动回收记录页的数据
//    auto rec_page = 0; //
//    auto rec_offset = 0;
//}

template<typename key_t>
page_id_t bplus_tree<key_t>::down_to_leaf(key_t key) {
    if(root == page::ERROR_PAGE) return page::ERROR_PAGE;
    auto cur_id = root;
    while(true) {
        const auto &cur = read_op<index_page>(cur_id);
        if(cur.is_leaf()) break;
        auto pos = cur.find_first_gt(key);
        cur_id = cur.get_ch(pos);
    }
    return cur_id;
}

template<typename key_t>
void bplus_tree<key_t>::traverse(page_id_t leaf_id, page_off_t pos, key_t ub, std::vector<page_off_t> &records, int need) {
    if(!need || leaf_id == page::ERROR_PAGE) return;
    while(need) {
        const auto &leaf = read_op<index_page>(leaf_id);
        if(pos >= leaf.get_key_cnt()) {
            auto next = leaf.get_next();
            if(next == page::ERROR_PAGE) return;
            pos = 0;
            leaf_id = next;
        } else {
            if(leaf.get_key(pos) > ub) return;
            auto ch = leaf.get_ch(pos);
            auto is_lazy = (ch == page::ERROR_PAGE);
            if(!is_lazy) { // 可能是LAZY标记过的记录，那就跳过该部分
                records.push_back(ch);
                --need;
            }
            ++pos;
        }
    }
}


template<typename key_t>
void bplus_tree<key_t>::insert_parent(page_id_t l_node_id, key_t key, page_id_t r_node_id) {
    if(l_node_id == root) {
        root = bio.alloc<index_page>(bplus_page<key_t>::ROOT | bplus_page<key_t>::INTERNAL);
        head.set_root(root);
        write_op<index_page>(root).
                append_ch(l_node_id).append_key(key).append_ch(r_node_id);
        auto &l_node = write_op<index_page>(l_node_id);
        l_node.set_parent(root);
        auto magic = l_node.get_magic();
        magic ^= index_page::ROOT;
        l_node.set_magic(magic);
        write_op<index_page>(r_node_id).set_parent(root);
        return;
    }
    auto parent_id = read_op<index_page>(l_node_id).get_parent();
    auto &parent = write_op<index_page>(parent_id);
    if(!read_op<index_page>(parent_id).is_full()) {
        auto pos = parent.find_ch(l_node_id);
        parent.insert_key(pos, key);
        parent.insert_ch(pos + 1, r_node_id);
        write_op<index_page>(r_node_id).set_parent(parent_id);
    } else {
        auto new_parent_id = bio.alloc<index_page>(index_page::INTERNAL);
        auto new_key = split(parent_id, new_parent_id, key, l_node_id, r_node_id);
        insert_parent(parent_id, new_key, new_parent_id);
    }
}


template<typename key_t>
key_t bplus_tree<key_t>::split(page_id_t full_node_id, page_id_t new_node_id, key_t key, page_off_t rec) {
    auto &full_node = write_op<index_page>(full_node_id);
    auto &new_node = write_op<index_page>(new_node_id);
    auto temp_node = mem_node<key_t>(bplus_page<key_t>(full_node));
    temp_node.insert_notfull(key,rec);
    full_node.clear();
    for(auto i = 0; i < index_page::split_point; ++i) {
        full_node
            .append_ch(temp_node.get_ch(i))
            .append_key(temp_node.get_key(i));
    }
    for(auto i = index_page::split_point; i < index_page::order; ++i) {
        new_node
            .append_ch(temp_node.get_ch(i))
            .append_key(temp_node.get_key(i));
    }
    auto next_id = full_node.get_next();
    auto parent_id = full_node.get_parent();

    full_node.set_next(new_node_id);

    new_node.set_next(next_id);
    new_node.set_parent(parent_id);

    return new_node.get_key(0);
}


template<typename key_t>
key_t bplus_tree<key_t>::split(page_id_t full_node_id, page_id_t new_node_id,
                                key_t key, page_id_t l_ch_id, page_id_t r_ch_id) {
    //        auto temp_node = bplus_page(read_op<index_page>(full_node_id));  // FIXED temp风险
    auto &full_node = write_op<index_page>(full_node_id);
    auto &new_node = write_op<index_page>(new_node_id);
    auto temp_node = mem_node<key_t>(bplus_page<key_t>(full_node));
    // temp_node.set_magic(bplus_page::LEAF);
    auto pos = temp_node.find_ch(l_ch_id);
    temp_node.insert_ch(pos+1, r_ch_id);
    temp_node.insert_key(pos,key);
    new_node.set_parent(full_node.get_parent());
    full_node.clear();
    for(auto i = 0; i < index_page::split_point; ++i) {
        full_node.append_ch(temp_node.get_ch(i));
        if(i == index_page::split_point - 1) break;
        full_node.append_key(temp_node.get_key(i));
    }
    auto mid_k = temp_node.get_key(index_page::split_point - 1);
    for(auto i = index_page::split_point; i < index_page::order; ++i) {
        new_node.append_ch(temp_node.get_ch(i))
                .append_key(temp_node.get_key(i));
    }
    new_node.append_ch(temp_node.get_ch(index_page::order));
    auto ch_cnt = new_node.get_child_cnt();
    for(auto i = 0; i != ch_cnt; ++i) {
        auto ch_id = new_node.get_ch(i);
        write_op<index_page>(ch_id).set_parent(new_node_id);
    }
    return mid_k;
}

template<typename key_t>
void bplus_tree<key_t>::remove(page_id_t node_id, key_t key, page_id_t ch_id) {
    auto &node = write_op<index_page>(node_id);
    auto key_pos = node.find_first_ge(key);
    auto ch_pos = node.find_ch(ch_id);
    node.delete_key(key_pos);
    node.delete_ch(ch_pos); // 只是简单移除指针
    if(node_id == root) {
        remove_root_condition();
        return;
    }
    if(node.get_key_cnt() >= index_page::min_key_size) {
        return;
    }
    auto parent_id = node.get_parent();
    const auto &parent = read_op<index_page>(parent_id);
    auto neighbor_pos = int64_t(parent.find_ch(node_id)) - 1; // FIXED: 无符号0 BUG
    auto mid_k_pos = (neighbor_pos == -1) ? 0 : neighbor_pos;
    auto mid_k = parent.get_key(mid_k_pos);
    auto neighbor_id = parent.get_ch(neighbor_pos == -1 ? 1 : neighbor_pos);
    if(read_op<index_page>(neighbor_id).get_key_cnt() + node.get_key_cnt() < index_page::order) {
        coalesce_condition(node_id, neighbor_id, neighbor_pos, mid_k);
    } else {
        overflow_condition(node_id, neighbor_id, neighbor_pos, mid_k_pos, key);
    }
}

template<typename key_t>
void bplus_tree<key_t>::coalesce_condition(page_id_t node, page_id_t neighbor, int64_t neighbor_pos, key_t mid_k) {
    if(neighbor_pos == -1) std::swap(node,neighbor);
    if(!read_op<index_page>(node).is_leaf()) {
        write_op<index_page>(neighbor).append_key(mid_k);
        auto key_size = read_op<index_page>(node).get_key_cnt();
        auto child_size = read_op<index_page>(node).get_child_cnt();
        for(auto i = 0; i != key_size; ++i) {
            write_op<index_page>(neighbor).append_key(
                    read_op<index_page>(node).get_key(i));
        }
        for(auto i = 0; i != child_size; ++i) { // 性能致命打击
            write_op<index_page>(neighbor).append_ch(read_op<index_page>(node).get_ch(i));
            write_op<index_page>(read_op<index_page>(node).get_ch(i)).set_parent(neighbor);
        }
    } else {
        auto key_cnt = read_op<index_page>(node).get_key_cnt();
        for(auto i = 0; i != key_cnt; ++i) {
            write_op<index_page>(neighbor)
                    .append_ch(read_op<index_page>(node).get_ch(i))
                    .append_key(read_op<index_page>(node).get_key(i));
        }
        write_op<index_page>(neighbor).set_next(read_op<index_page>(node).get_next());
    }
    write_op<index_page>(node).clear();
    remove(read_op<index_page>(node).get_parent(),mid_k,node);
    bio.dealloc<index_page>(node); // 可放心回收
}

template<typename key_t>
void bplus_tree<key_t>::remove_root_condition() {
    if(read_op<index_page>(root).get_key_cnt() > 0) return;
    if(read_op<index_page>(root).is_leaf()) {
        bio.dealloc<index_page>(root);
        head.set_root(root = page::ERROR_PAGE);
        return;
    } else {
        auto new_root = read_op<index_page>(root).get_ch(0);
        write_op<index_page>(root).clear();
        bio.dealloc<index_page>(root);
        head.set_root(root = new_root);
        write_op<index_page>(root).set_magic(
                read_op<index_page>(root).get_magic() | bplus_page<key_t>::ROOT);
        write_op<index_page>(root).set_parent(page::ERROR_PAGE);
    }
}

template<typename key_t>
void bplus_tree<key_t>::overflow_condition(page_id_t node, page_id_t neighbor,
        int neighbor_pos, int key_pos, key_t key) {
    if(neighbor_pos != -1) { // OK
        auto ch_cnt = read_op<index_page>(neighbor).get_child_cnt();
        auto ch = read_op<index_page>(neighbor).get_ch(ch_cnt-1);
        write_op<index_page>(node).insert_ch(0, ch); // OK
        if(read_op<index_page>(node).is_leaf()) { // OK
            auto key_cnt = read_op<index_page>(neighbor).get_key_cnt();
            auto tmp_key = read_op<index_page>(neighbor).get_key(key_cnt-1);
            write_op<index_page>(node).insert_key(0,tmp_key);
            auto parent = read_op<index_page>(node).get_parent();
            tmp_key = read_op<index_page>(node).get_key(0);
            write_op<index_page>(parent).set_key(key_pos,tmp_key);
        } else { // OK
            auto node_ch = read_op<index_page>(node).get_ch(0);
            write_op<index_page>(node_ch).set_parent(node);
            write_op<index_page>(node).insert_key(0,key);
            auto parent = read_op<index_page>(node).get_parent();
            auto key_cnt = write_op<index_page>(neighbor).get_key_cnt();
            auto tmp_key = write_op<index_page>(neighbor).get_key(key_cnt-1);
            write_op<index_page>(parent).set_key(key_pos,tmp_key);
        }
        ch_cnt = read_op<index_page>(neighbor).get_child_cnt();
        auto key_cnt = read_op<index_page>(neighbor).get_key_cnt();
        write_op<index_page>(neighbor).delete_key(key_cnt-1);
        write_op<index_page>(neighbor).delete_ch(ch_cnt-1);
    } else {
        write_op<index_page>(node).append_ch(read_op<index_page>(neighbor).get_ch(0));
        if(read_op<index_page>(node).is_leaf()) { // ok
            auto tmp_key = read_op<index_page>(neighbor).get_key(0);
            auto tmp_key2 = read_op<index_page>(neighbor).get_key(1);
            write_op<index_page>(node).append_key(tmp_key);
            auto parent = read_op<index_page>(node).get_parent();
            write_op<index_page>(parent).set_key(key_pos, tmp_key2);
        } else { // ok
            auto ch = read_op<index_page>(node).get_ch(0);
            auto parent = read_op<index_page>(node).get_parent();
            write_op<index_page>(ch).set_parent(node);
            write_op<index_page>(node).append_key(key);
            auto tmp_key = read_op<index_page>(neighbor).get_key(0);
            write_op<index_page>(parent).set_key(key_pos,tmp_key);
        }
        write_op<index_page>(neighbor).delete_key(0);
        write_op<index_page>(neighbor).delete_ch(0);
    }
}

template<typename key_t>
page_id_t bplus_tree<key_t>::get_root() {
    return root;
}

template<typename key_t>
io_manager &bplus_tree<key_t>::get_bio() {
    return bio;
}

template<typename key_t>
void bplus_tree<key_t>::remove(key_t key) {
    auto leaf = down_to_leaf(key);
    if(leaf == page::ERROR_PAGE) return;
    auto pos = read_op<index_page>(leaf).find_key(key);
    if(pos == read_op<index_page>(leaf).get_key_cnt()) return; // 确实不存在
    auto rec_pos = read_op<index_page>(leaf).get_ch(pos);
    if(rec_pos == page::ERROR_PAGE) return; // 早已打上了LAZY标记
    write_op<index_page>(leaf).set_ch(pos, page::ERROR_PAGE); // LAZY delete
}

template<typename key_t>
bplus_tree<key_t>::bplus_tree(io_manager &bio, rec_manager &rm, head_page &head):
        bio(bio), rm(rm), head(head) { root = head.get_root(); }

template<typename key_t>
void bplus_tree<key_t>::set_tx(tx *new_tx) {
    cur_tx = new_tx;
}

template<typename key_t>
bool bplus_tree<key_t>::has_tx() {
    return cur_tx != nullptr;
}

template<typename key_t>
tx &bplus_tree<key_t>::get_tx() {
    return *cur_tx;
}

template<typename key_t>
head_page &bplus_tree<key_t>::get_head() {
    return head;
}

template<typename key_t>
void bplus_tree<key_t>::set_root(page_id_t new_root) {
    root = new_root;
}


#endif //VSQL_BPLUS_TREE_H
