#ifndef VSQL_PARSER_H
#define VSQL_PARSER_H

#include <functional>
#include "../schema/type.h"
#include "../sqlite/sqlite3.h"
#include "value.h"
#include "table.h"
#include "record.h"

// 专门对付sqlite-API层
// 目标是绝大多数API对接对放在这里，避免对我正常项目的耦合（除了迭代器这种乱七八糟的。。）
class parser {
public:

    // 针对value和sqlite3_value

    // 通过简单的表驱动来实现数值输出
    static void decode(sqlite3_context* ctx, col_type type, value v);
    static void encode(sqlite3_value *sv, col_type type, value &v);


    table &belong_to;

    explicit parser(table &table);


    record raw2rec(const char *raw, size_t raw_size); // 构造record的简单方法， TODO 改为record(const char *raw)直接构造？
    size_t rec2raw(const record &rec, char *&raw); // 用于放置到rec_page，对应于bplus.insert接口 【注意】:可能引起内存泄漏

};

#endif //VSQL_PARSER_H
