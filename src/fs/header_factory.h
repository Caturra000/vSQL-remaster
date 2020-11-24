#ifndef VSQL_HEADER_FACTORY_H
#define VSQL_HEADER_FACTORY_H

#include <unordered_map>
#include "../page/head_page.h"
#include "disk.h"

// 用于为维护各种表的head_page
class header_factory {
private:
    std::unordered_map<std::string, head_page> mapping;
    header_factory() = default;
public:
    static header_factory& get_factory();
    head_page& get_header(const std::string &file_name); // 用于程序首次磁盘->内存，后面使用引用持有而不经过冗余的哈希查找
    header_factory(const header_factory &) = delete;
    header_factory& operator = (const header_factory &) = delete;
    ~header_factory();
};



#endif //VSQL_HEADER_FACTORY_H
