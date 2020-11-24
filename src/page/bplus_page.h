#ifndef VSQL_BPLUS_PAGE_H
#define VSQL_BPLUS_PAGE_H


#include "page.h"


/*
 * 大概长这样
 ----------------------------------------------------------------
| MAGIC | PARENT | KEY_CNT | CHILD_CNT | --KEYS-- | --CHILDREN-- |
 ----------------------------------------------------------------
|←               FIXED                →|
 *
 */

// 其实一个类就足够表示内部和叶子了
// 注意叶子的children是每个64位，因为记录查询需要额外的offset，在IO时需要特别留意，从child中分离page id和offset
// todo 引入comparator
template<typename key_t>
class bplus_page : public page {
protected:
    void swap(bplus_page &that);
public:

    explicit bplus_page(uint32_t magic = page::UNDEFINED,      // TODO 构造函数部分有点瑕疵
                        page_off_t parent = page::ERROR_PAGE);
    explicit bplus_page(char *&&temp_data);
    bplus_page(const bplus_page &that);
    bplus_page(bplus_page &&that);
    bplus_page& operator = (const bplus_page &that);


    // 不可选择与运算歧义的数字
    // 只允许 1 2 4 8
    enum magic_type {
        ROOT     = 0x1000, // 兼容LEAF / INTERNAL
        LEAF     = 0x0200,
        INTERNAL = 0x0400, // 与LEAF互斥
    };

    // 通过constexpr推断避免以后添加更多的域 / 更改类型导致的hardcode，且无运行时成本
    accessor(parent,    page_id_t, field_end_magic);
    accessor(key_cnt,   uint32_t,  field_end_parent);
    accessor(child_cnt, uint32_t,  field_end_key_cnt);

    // 任何改动只需修改这里
    static const size_t end_fixed = field_end_child_cnt;

    // 求出最优的平衡因子
    static constexpr size_t order =
            (PAGE_SIZE - end_fixed + sizeof(key_t)) / (sizeof(key_t) + sizeof(page_off_t));
private:
    partition(keys, key_t, end_fixed, field_start_keys + sizeof(key_t)*(order-1));
    partition(children, page_off_t, field_end_keys,     // 叶子的children是指向记录,需要pageid|offset支持
    field_start_children + sizeof(page_off_t)*order);   // 可能有部分未填充的field，因此不要默认结尾就是PAGE_NAX

    // PS.为了兼容page_off和page_id的寻址,采用低4字节为page_id，高4字节为offset，这样就不怕忘记解析而把page_off看成offset
public:

    static constexpr size_t split_point = order >> 1;
    static constexpr size_t min_key_size = (order+1 >> 1) - 1;

    bool is_root() const;
    bool is_leaf() const;
    bool is_full() const;


    // SPECIAL FOR LEAF
    // 后面再搞个子类？ B+树的结点行为杂糅不好操作

    page_off_t get_next() const;
    void set_next(page_off_t next); // next不统计在child_cnt当中，但是在叶子上固定占有children最后一字节
    void insert_notfull(key_t key, page_off_t ch);


    // 其实key和child在基础操作上仅有小部分差异
    // 可以用内部类或者在宏上加强封装
    // 但是我拒绝

    bplus_page& append_key(key_t key);
    bplus_page& append_ch(page_off_t ch);
    page_off_t get_ch(size_t pos) const;
    key_t get_key(size_t pos) const;
    void insert_key(int pos, key_t key); // TODO 可以用memmove优化，后面再改
    void insert_ch(int pos, page_off_t ch);
    void set_key(int pos, key_t key);
    void set_ch(int pos, page_off_t ch);
    void delete_key(int pos);
    void delete_ch(int pos);
    void clear();

    // pos相关

    size_t find_ch(page_off_t ch) const; // 失败则返回哨兵
    size_t find_key(key_t key) const;
    size_t find_first_ge(key_t key) const;
    size_t find_first_gt(key_t key) const;

};







template<typename key_t>
bplus_page<key_t>::bplus_page(uint32_t magic, page_off_t parent): page(magic) {
    set_parent(parent);
    set_key_cnt(0);
    set_child_cnt(0);
    set_next(0);
}

template<typename key_t>
bplus_page<key_t>::bplus_page(char *&&temp_data): page(std::move(temp_data)) { }

template<typename key_t>
bplus_page<key_t> &bplus_page<key_t>::operator=(const bplus_page &that) {
    if(this == &that) return *this;
    if(that.get_magic() == page::FAST_COPY) {
        memcpy(data, that.data, end_fixed);
    } else {
        memcpy(data, that.data, PAGE_SIZE);
    }
    return *this;
}

template<typename key_t>
bool bplus_page<key_t>::is_root() const {
    return (get_magic() & 0xF000) == ROOT;
}

template<typename key_t>
bool bplus_page<key_t>::is_leaf() const {
    return (get_magic() & 0x0F00) == LEAF;
}

template<typename key_t>
bool bplus_page<key_t>::is_full() const {
    return get_key_cnt() >= order-1;
}

template<typename key_t>
page_off_t bplus_page<key_t>::get_ch(size_t pos) const {
    return children[pos];
}

template<typename key_t>
key_t bplus_page<key_t>::get_key(size_t pos) const {
    return keys[pos];
}

template<typename key_t>
page_off_t bplus_page<key_t>::get_next() const {
    if(is_leaf()) {
        return children[order];
    }
    return page::ERROR_PAGE;
}

template<typename key_t>
void bplus_page<key_t>::set_next(page_off_t next) {
    if(is_leaf()) {
        children[order] = next;
    }
}

template<typename key_t>
void bplus_page<key_t>::insert_notfull(key_t key, page_off_t ch) {
    if(is_leaf()) {
        auto pos = find_first_gt(key);
        insert_key(pos,key);
        insert_ch(pos,ch);
    }
}

template<typename key_t>
bplus_page<key_t> &bplus_page<key_t>::append_key(key_t key) {
    auto pos = get_key_cnt();
    keys[pos] = key;
    set_key_cnt(pos+1);
    return *this;
}

template<typename key_t>
bplus_page<key_t> &bplus_page<key_t>::append_ch(page_off_t ch) {
    auto pos = get_child_cnt();
    children[pos] = ch;
    set_child_cnt(pos+1);
    return *this;
}

template<typename key_t>
void bplus_page<key_t>::insert_key(int pos, key_t key) {
    auto cnt = get_key_cnt();
    for(auto i = cnt; i > pos; --i) {
        keys[i] = keys[i-1];
    }
    keys[pos] = key;
    set_key_cnt(cnt+1);
}

template<typename key_t>
void bplus_page<key_t>::insert_ch(int pos, page_off_t ch) {
    auto cnt = get_child_cnt();
    for(auto i = cnt; i > pos; --i) {
        children[i] = children[i-1];
    }
    children[pos] = ch;
    set_child_cnt(cnt+1);
}

template<typename key_t>
void bplus_page<key_t>::set_key(int pos, key_t key) {
    keys[pos] = key;
}

template<typename key_t>
void bplus_page<key_t>::set_ch(int pos, page_off_t ch) {
    children[pos] = ch;
}

template<typename key_t>
void bplus_page<key_t>::delete_key(int pos) {
    auto cnt = get_key_cnt() - 1;
    for(auto i = pos; i < cnt; ++i) {
        keys[i] = keys[i+1];
    }
    set_key_cnt(cnt);
}

template<typename key_t>
void bplus_page<key_t>::delete_ch(int pos) {
    auto cnt = get_child_cnt() - 1;
    for(auto i = pos; i < cnt; ++i) {
        children[i] = children[i+1];
    }
    set_child_cnt(cnt);
}

template<typename key_t>
void bplus_page<key_t>::clear() {
    set_key_cnt(0);
    set_child_cnt(0);
}

template<typename key_t>
size_t bplus_page<key_t>::find_ch(page_off_t ch) const {
    auto cnt = get_child_cnt();
    for(auto i = 0; i != cnt; ++i) {
        if(ch == children[i]) return i;
    }
    return cnt;
}

template<typename key_t>
size_t bplus_page<key_t>::find_key(key_t key) const {
    auto pos = find_first_ge(key);
    auto cnt = get_key_cnt();
    if(pos == cnt) return cnt;
    if(keys[pos] != key) return cnt;
    return pos;
}

template<typename key_t>
size_t bplus_page<key_t>::find_first_ge(key_t key) const {
    auto cnt = get_key_cnt();
    auto lo = 0;
    auto hi = cnt-1;
    if(cnt == 0) return cnt; // ERROR
    while(lo < hi) {
        auto mid = lo + (hi-lo >> 1);
        if(keys[mid] >= key) hi = mid;
        else lo = mid+1;
    }
    return keys[lo] >= key ? lo : cnt;
}

template<typename key_t>
size_t bplus_page<key_t>::find_first_gt(key_t key) const {
    auto cnt = get_key_cnt();
    auto lo = 0;
    auto hi = cnt-1;
    if(cnt == 0) return cnt; // ERROR
    while(lo < hi) {
        auto mid = lo + (hi-lo >> 1);
        if(keys[mid] > key) hi = mid;
        else lo = mid+1;
    }
    return keys[lo] > key ? lo : cnt;
}

template<typename key_t>
void bplus_page<key_t>::swap(bplus_page &that) {
    page::swap(that);
    std::swap(keys, that.keys);
    std::swap(children, that.children);
}

template<typename key_t>
bplus_page<key_t>::bplus_page(const bplus_page &that):
        page(that) { } // keys children还是原来的指向

template<typename key_t>
bplus_page<key_t>::bplus_page(bplus_page &&that):
        page(std::move(that)), keys(that.keys), children(that.children) { // 指针跟随窃取
    that.keys = nullptr;
    that.children = nullptr;
}


#endif //VSQL_BPLUS_PAGE_H
