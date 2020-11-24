#ifndef VSQL_FS_H
#define VSQL_FS_H


// 一个问题，fs需要模板，因为bplus_tree要求


#include "bplus_tree.h"
#include "binder.h"

// 用于描述表中文件存储相关的信息
// 包括外存索引的具体形式和交互接口、表文件位置等
class fs {
    using bplus_index = bplus_tree<pri_key_t>;
private:
    std::string file_name;
    free_manager &fm;
    io_manager &io;
    rec_manager &rm;
    bplus_index b_index;
    // head_page   &header;

public:
    explicit fs(const std::string& file_name);
    bplus_index& get_index();
    void start_transaction();
    void recover(); // 只适用于崩溃恢复（单线程写）
    void commit();
    // get io rm...
};

#endif //VSQL_FS_H

