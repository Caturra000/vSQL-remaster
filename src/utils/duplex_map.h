#ifndef VSQL_DUPLEX_MAP_H
#define VSQL_DUPLEX_MAP_H

#include <cstddef>
#include <vector>
#include <unordered_map>

// 实现一个【特殊用途】的快速双向映射，
// 只适用于数值类型
// 第一个类型范围较大（随便多大），且小数较多（fast_capacity）
// 第二个类型范围足够小（小于百万级）
// 两者范围均不小于0
// 因为都是已知范围的，所以不进行越界检查
template <typename first_type, typename second_type,
        size_t first_fast_capacity, size_t second_capacity>
class duplex_map {
    std::vector<second_type> tiny_mapping; // 优化小数访问
    std::unordered_map<first_type, second_type> mapping;
    std::vector<first_type> mapping_rev;  // 反向映射
    const first_type first_default;       // first_default表示【rev查找】时找不到默认返回的值
    const second_type second_default;

public:

    duplex_map(first_type first_error, second_type second_error);
    void create_mapping(first_type first, second_type second);
    second_type find(first_type key);
    first_type find_rev(second_type key); // 不进行 大于 second capacity 的检查
    void erase_mapping(first_type first);
    void erase_mapping_rev(second_type second);
};

template<typename first_type, typename second_type,
        size_t first_fast_capacity, size_t second_capacity>
duplex_map<first_type, second_type, first_fast_capacity, second_capacity>
        ::duplex_map(first_type first_error, second_type second_error)
            : first_default(first_error), second_default(second_error),
                tiny_mapping(first_fast_capacity, second_error),
                mapping_rev(second_capacity, first_error) { }

template<typename first_type, typename second_type,
        size_t first_fast_capacity, size_t second_capacity>
void duplex_map<first_type, second_type, first_fast_capacity, second_capacity>
        ::create_mapping(first_type first,second_type second) {
    if(first < first_fast_capacity) {
        tiny_mapping[first] = second;
    } else {
        mapping[first] = second;
    }
    mapping_rev[second] = first;
}

template<typename first_type, typename second_type,
        size_t first_fast_capacity, size_t second_capacity>
second_type duplex_map<first_type, second_type, first_fast_capacity, second_capacity>
        ::find(first_type key) {
    if(key < first_fast_capacity) {
        return tiny_mapping[key];
    }
    auto it = mapping.find(key);
    if(it != mapping.end()) return it->second;
    return second_default;
}

template<typename first_type, typename second_type,
        size_t first_fast_capacity, size_t second_capacity>
first_type duplex_map<first_type, second_type, first_fast_capacity, second_capacity>
        ::find_rev(second_type key) {
    return mapping_rev[key];
}

template<typename first_type, typename second_type,
        size_t first_fast_capacity, size_t second_capacity>
void duplex_map<first_type, second_type, first_fast_capacity, second_capacity>
        ::erase_mapping(first_type first) {
    auto second = find(first);
    if(second == second_default) return;
    if(first < first_fast_capacity) {
        tiny_mapping[first] = second_default;
    } else {
        mapping.erase(first);
    }
    mapping_rev[second] = first_default;
}

template<typename first_type, typename second_type,
        size_t first_fast_capacity, size_t second_capacity>
void duplex_map<first_type, second_type, first_fast_capacity, second_capacity>
        ::erase_mapping_rev(second_type second) {
    auto first = find_rev(second);
    if(first == first_default) return;
    if(first < first_fast_capacity) {
        tiny_mapping[first] = second_default;
    } else {
        mapping.erase(first);
    }
    mapping_rev[second] = first_default;
}

#endif //VSQL_DUPLEX_MAP_H
