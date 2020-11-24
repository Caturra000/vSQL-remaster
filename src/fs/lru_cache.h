#ifndef VSQL_LRU_CACHE_H
#define VSQL_LRU_CACHE_H

#include "../utils/array_list.h"
#include "../defs/defs.h"

// 抽象的lru类，自身并没有任何实质数据，嵌入使用
template <typename T>
class lru_cache {
private:
    array_list<T> list;
    T             head_pos;
public:
    explicit lru_cache(T size);
    void access(T pos);
    T least();
    T size();
    ~lru_cache() {}
};

template<typename T>
lru_cache<T>::lru_cache(T size)
    : list(size),  head_pos(0) { static_assert(std::is_integral<T>::value, "T must be integer type"); }

template<typename T>
void lru_cache<T>::access(T pos) {
    if(pos == head_pos) return;
    auto prev_pos = list.prev(pos);
    auto next_pos = list.next(pos);
    list.next(prev_pos) = next_pos;
    list.prev(next_pos) = prev_pos;
    auto tail_pos = least();
    list.prev(head_pos) = pos;
    list.next(tail_pos) = pos;
    list.next(pos) = head_pos;
    list.prev(pos) = tail_pos;
    head_pos = pos;
}

template<typename T>
T lru_cache<T>::least() {
    return list.prev(head_pos);
}



template<typename T>
T lru_cache<T>::size() {
    return list.size();
}

#endif //VSQL_LRU_CACHE_H
