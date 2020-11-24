#ifndef VSQL_MEM_NODE_H
#define VSQL_MEM_NODE_H

#include <vector>
#include "bplus_page.h"

template<typename key_t>
class mem_node {
    std::vector<page_off_t> children;
    std::vector<key_t> keys;
public:
    explicit mem_node(const bplus_page<key_t> &page);

    key_t get_key(int pos) const;
    page_off_t get_ch(int pos) const;
    void insert_notfull(key_t key, page_off_t rec);
    void set_key(int pos, key_t key);
    void set_ch(int pos, page_off_t ch);
    int find_ch(page_off_t ch);
    void insert_ch(int pos, page_off_t ch);
    void insert_key(int pos, key_t key);
};




template<typename key_t>
mem_node<key_t>::mem_node(const bplus_page<key_t> &page) {
    auto key_cnt = page.get_key_cnt();
    auto ch_cnt = page.get_child_cnt();
    for(auto i = 0; i != key_cnt; ++i) {
        keys.push_back(page.get_key(i));
    }
    for(auto i = 0; i != ch_cnt; ++i) {
        children.push_back(page.get_ch(i));
    }
}

template<typename key_t>
key_t mem_node<key_t>::get_key(int pos) const {
    return keys[pos];
}

template<typename key_t>
page_off_t mem_node<key_t>::get_ch(int pos) const {
    return children[pos];
}

template<typename key_t>
void mem_node<key_t>::insert_notfull(key_t key, page_off_t rec) {
    auto i = 0;
    for(; i != keys.size() && keys[i] < key; ++i);
    keys.insert(keys.begin() + i, key);
    children.insert(children.begin() + i, rec);
}

template<typename key_t>
void mem_node<key_t>::set_key(int pos, key_t key) {
    keys[pos] = key;
}

template<typename key_t>
void mem_node<key_t>::set_ch(int pos, page_off_t ch) {
    children[pos] = ch;
}

template<typename key_t>
int mem_node<key_t>::find_ch(page_off_t ch) {
    for(auto i = 0; i != children.size(); ++i) {
        if(children[i] == ch) return i;
    }
    return children.size();
}

template<typename key_t>
void mem_node<key_t>::insert_ch(int pos, page_off_t ch) {
    children.insert(children.begin() + pos, ch);
}

template<typename key_t>
void mem_node<key_t>::insert_key(int pos, key_t key) {
    keys.insert(keys.begin() + pos, key);
}

#endif //VSQL_MEM_NODE_H
