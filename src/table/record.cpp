#include "record.h"

#include <utility>

row_id_t record::get_rowid() const {
    return values[0].i64; // TODO 不太严谨，尽量保证v默认初始化
}

value record::get(int pos) const {
    return values[pos];
}

record::~record() {
    if(temp) return;
    for(auto &i : var_info) {
        delete values[std::get<0>(i)].ptr;
    }
}

record::var_ptr record::make_var_ptr(column_id_t cid, offset_t off, size_t len) {
    return record::var_ptr(cid,off,len);
}

//record::record(const std::vector<value> &values,
//               const std::vector<var_ptr> &var_info,
//               size_t raw_size)
//        : values(values), var_info(var_info), raw_size(raw_size) { }

const record::var_ptr &record::get_var(int pos) const {
    return var_info[pos];
}

size_t record::get_raw_size() const {
    return raw_size;
}

record::record(std::vector<value> values,
               std::vector<var_ptr> var_info,
               size_t raw_size, bool temp)
        : values(std::move(values)), var_info(std::move(var_info)), raw_size(raw_size), temp(temp) { }
