#ifndef VSQL_ARRAY_LIST_H
#define VSQL_ARRAY_LIST_H

#include <utility>
#include <vector>
#include "tools.h"
#include "array.h"

// 简单的定长链表
template<typename T>
class array_list {
    using list_node_t = std::pair<T,T>;
    array<list_node_t> list;
    T fixed_size;
public:
    array_list() = delete;
    array_list(T size);
    force_inline T& prev(T pos);
    force_inline T& next(T pos);
    force_inline T size();
};

template<typename T>
array_list<T>::array_list(T size)
        : fixed_size(size), list(size) {
    for(T i = 0; i < size; ++i) {
        prev(i) = i - 1;
        next(i) = i + 1;
    }
    prev(0) = size-1;
    next(size-1) = 0;
}

template<typename T>
T& array_list<T>::prev(T pos) {
    return list[pos].first;
}

template<typename T>
T& array_list<T>::next(T pos) {
    return list[pos].second;
}

template<typename T>
T array_list<T>::size() {
    return fixed_size;
}


#endif //VSQL_ARRAY_LIST_H
