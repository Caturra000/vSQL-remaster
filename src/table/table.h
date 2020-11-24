#ifndef VSQL_TABLE_H
#define VSQL_TABLE_H


#include <vector>
#include "../schema/schema.h"
#include "../fs/fs.h"
#include "../api/vtable.h"



/**
 * 关于主键：列为0只允许int64以内的整型，虽然浮点和varchar理论上也可以，但暂时不支持
 * 如果不声明主键？先不管
 * 字符串支持方案给出了
 * char和varchar目前的实现方法相同，因为我不打算加范围上的约束
 * boolean tinyint smallint 默认也是4字节int32，我也不知道3字节类型是怎样快速实现的
 * decimal是double浮点
 */

// 整个表的顶级接口
class table {
    // lock / log
    sqlite3_vtab vtab; // 必须放在首位
    schema _schema;
    fs _fs;




public:
//    table(const std::string &table_name, const std::string &column_args);
//    table(const std::string &table_name, const std::string &column_args,
//          const std::string &key_args);
    table(const std::string &table_name, const std::vector<std::string> &args);
    schema& get_schema();
    fs& get_fs();

    // 要不加个rec_parser类

//    void add_record(int argc, sqlite3_value **argv);
//    void parse_raw_data(const void *raw_rec);


    void remove(int64_t key);

};


#endif //VSQL_TABLE_H


