//
// Created by Caturra on 2020/3/6.
//

#ifndef VSQL_ARRAY_H
#define VSQL_ARRAY_H

#include <cstdint>

/**
 * 简单的定长数组
 * 用于把数据放到堆上，防止普通数组可能引起的爆栈/忘记delete
 */
template<typename T>
class array {
    size_t length;
    T      *data;
public:
    array();
    explicit array(size_t size);
    ~array();
    size_t size();
    T& operator [] (size_t pos);
};

template<typename T>
array<T>::array()
    : length(0), data(nullptr) { }

template<typename T>
array<T>::array(size_t size)
    : length(size), data(new T[size]) { }

template<typename T>
array<T>::~array() {
    delete[] data;
}

template<typename T>
size_t array<T>::size() {
    return length;
}

template<typename T>
T &array<T>::operator [] (size_t pos) {
    return data[pos];
}


#endif //VSQL_ARRAY_H
