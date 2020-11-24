#ifndef VSQL_FIXED_MAP_H
#define VSQL_FIXED_MAP_H

#include "array.h"

template <typename K,typename V>
class fixed_map {
    array<K> head, next, to;
    array<V> val;
    size_t count;
    fixed_map(size_t size);
    void add(const K &key, const V &val);
};

template<typename K, typename V>
fixed_map<K, V>::fixed_map(size_t size)
    : head(size), next(size), to(size), val(size), count(0) { }

template<typename K, typename V>
void fixed_map<K, V>::add(const K &key, const V &val) {
}

#endif //VSQL_FIXED_MAP_H
