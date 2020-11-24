#ifndef VSQL_SCHEMA_H
#define VSQL_SCHEMA_H


#include <string>
#include <vector>
#include "column.h"
#include "id_gen.h"

// visible to sqlite_args, can parse the arguments


// 用于描述表的信息，分为 表属性、列属性、键属性
// 也用于直接对sqlite接口层的解析
class schema {
    std::string table_name;
    std::vector<column> columns;
    std::vector<column_id_t> keys;
    offset_t offset; // 记录所有column单位的固定偏移，专门对付可变长类型
    bool has_primary; // 如果是false,则是隐式primary_key
    column_id_t primary_key;
    id_gen generator;
public:
    schema(std::string table_name, const std::vector<std::string> &info);
    const std::string& get_table_name() const;
    size_t get_column_size() const;
    const column& get_column(int pos) const;
    const std::vector<column>& get_columns() const;
    size_t get_offset() const;
    bool primary_hidden();
    column_id_t get_primary();
    long long generate_id();
};

#endif //VSQL_SCHEMA_H
