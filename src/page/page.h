#ifndef VSQL_PAGE_H
#define VSQL_PAGE_H

#include "../defs/defs.h"
#include <cstring>

// 为data分出单独一个小区域，并提供setter/getter
// start为data上的偏移,单位/1字节，允许表达式运算形式
#define accessor(name, type, start)                                \
static constexpr size_t field_size_##name = sizeof(type);          \
static constexpr size_t field_start_##name = start;                \
static constexpr size_t field_end_##name = start + sizeof(type);   \
void set_##name(type name) {                                       \
    *reinterpret_cast<type*>(data+field_start_##name) = name;      \
}                                                                  \
type get_##name() const {                                          \
    return *reinterpret_cast<type*>(data+field_start_##name);      \
}

// 为data提供动态的连续一段区域，并提供一个基于data偏移的指针
// 使用此段需要private，因为type* name会暴露
#define partition(name, type, start, end)                   \
static constexpr size_t unit_size_##name = sizeof(type);    \
static constexpr size_t field_start_##name = start;         \
static constexpr size_t field_end_##name = end;             \
type* name {                                                \
    reinterpret_cast<type*>(data + field_start_##name)      \
};


/**
 * 约定：
 * 1.page的所有数据只存在于data，大小为固定的PAGE_SIZE，不添加其他多余的成员
 * 2.如果定义了其他的partition划分，需要额外的swap支持
 * 3.虽然各个子类用途划分明显，但还是需要定义magic_type，因为细分的类型实在是太多了
 * 4.有空的话实现以下fast_copy，可选项
 * 5.因为对象大小比较大，会对性能有一定影响，所以要完整实现移动语义
 */

// 目前期望是避免使用virtual，因为这些页结构有特殊性（比如真正的成员其实只有data），不用花费额外性能去支持多态

class page {
protected:
    char *data; // data设计为指针而不是[PAGE_SIZE]，提供更加灵活的资源转移，也防止了爆栈（内存也更难控制了点。。）
    void swap(page &that); // 用于支持copy and swap技巧
public:
    explicit page(uint32_t magic = UNDEFINED);
    page(const page &that);
    page(page &&that) noexcept;
    // 用于窃取指针，&&是要求显式声明右值才可使用
    // 这样避免了写入时为了高效而不得不暴露可写data的问题
    explicit page(char *&&temp_data); // 窃取指针，只用于fread之类的已分配data的函数，减少分配次数
    ~page();
    page& operator = (page that);
    accessor(magic, uint32_t, 0);
    static constexpr size_t fixed_end_if_free = field_end_magic;
    static const page_id_t ERROR_PAGE = 0;

    // 用于校验文件类型，目前只用16位
    enum magic_type {
        UNDEFINED = 0x0000,
        FREE      = 0xFFFF, // 被回收的无效的类型
        FAST_COPY = 0x8421, // 用于移动（伪）而非完全拷贝的临时类型
    };

    bool is_free() const;
    bool is_fast_copy() const;
    page_id_t get_next_free() const;
    void set_next_free(page_id_t next_free);
    const char* get_data() const;
    void overlap_from(FILE *file); // 提供直接写入的API
};




#endif //VSQL_PAGE_H
