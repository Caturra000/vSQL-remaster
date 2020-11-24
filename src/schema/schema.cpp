#include "schema.h"
#include "../utils/tools.h"

#include <cassert>
#include <utility>


const std::string &schema::get_table_name() const {
    return table_name;
}


size_t schema::get_column_size() const {
    return columns.size();
}

const column &schema::get_column(int pos) const {
    return columns[pos];
}

const std::vector<column> &schema::get_columns() const {
    return columns;
}

size_t schema::get_offset() const {
    return offset;
}

schema::schema(std::string table_name, const std::vector<std::string> &info)
        : table_name(std::move(table_name)), has_primary(false), primary_key(0) {
    column_id_t id = 0;
    offset = 0;
    for(const auto & col : info) {
        auto sp = string_utils::split(col, ' ');
        if(sp.size() < 2) continue; // 要么两个 要么三/四个
        auto col_name = col.substr(sp[0].first, sp[0].second - sp[0].first);
        auto type_name = col.substr(sp[1].first, sp[1].second - sp[1].first);
        auto type = col_type_handler::parse(type_name); // 认为合法
        if(type == col_type::UNKNOWN) continue;
        auto type_size = col_type_handler::size(type);
        offset += type_size;
        columns.emplace_back(id, 0, col_name, type);
        if(sp.size() > 2) { // 存在P 或者 K
            auto key_name = col.substr(sp[2].first, sp[2].second - sp[2].first);
            if(key_name == "primary") {
                if(has_primary) continue;
                if(type != col_type::INT && type != col_type::BIGINT) continue;
                primary_key = id;
                has_primary = true;
            } else if(key_name == "key") {
                if(type == col_type::CHAR || type == col_type::VARCHAR) continue; // 这些支持太麻烦了
                keys.push_back(id);
            }
        }
        ++id;
    }

    // 如果到最后还是 has_primary == false
    // 那就在【最后一列】的后边插入隐藏的列（现在没有UNIQUE / NOT NULL之类的选择，只能这样了）
    // 这样不用往前插又改id啥的
    // 但是要多一个rowid生成器才行
    if(!has_primary) {
        columns.emplace_back(id, 0, "__xilixili_gan_bei__", col_type::BIGINT);
        offset += col_type_handler::size(col_type::BIGINT);
        primary_key = id;
    }
}

bool schema::primary_hidden() {
    return !has_primary;
}

column_id_t schema::get_primary() {
    return primary_key;
}

long long schema::generate_id() {
    return generator.next();
}
