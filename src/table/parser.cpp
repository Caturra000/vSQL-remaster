#include <cstring>
#include "parser.h"

void parser::decode(sqlite3_context *ctx, col_type type, value v) {
    static std::function<void(sqlite3_context*,value)> mapping[] = {
            /*[UNKNOWN]  = */[](sqlite3_context *ctx, value v) { },
            /*[INT]      = */[](sqlite3_context *ctx, value v) { sqlite3_result_int(ctx,v.i32); },
            /*[CHAR]     = */[](sqlite3_context *ctx, value v) { sqlite3_result_text(ctx, v.ptr, -1, SQLITE_TRANSIENT); },
            /*[VARCHAR]  = */[](sqlite3_context *ctx, value v) { sqlite3_result_text(ctx, v.ptr, -1, SQLITE_TRANSIENT); },
            /*[BIGINT]   = */[](sqlite3_context *ctx, value v) { sqlite3_result_int64(ctx,v.i64); },
            /*[BOOLEAN]  = */[](sqlite3_context *ctx, value v) { sqlite3_result_int(ctx,v.i32); },
            /*[DOUBLE]   = */[](sqlite3_context *ctx, value v) { sqlite3_result_int(ctx,v.f64); },
            /*[SMALLINT] = */[](sqlite3_context *ctx, value v) { sqlite3_result_int(ctx,v.i32); },
            /*[TINYINT]  = */[](sqlite3_context *ctx, value v) { sqlite3_result_int(ctx,v.i32); },

    };
    mapping[type](ctx,v);
}

void parser::encode(sqlite3_value *sv, col_type type, value &v) {
    static std::function<void(sqlite3_value*, value&)> mapping[] = {
            /*[UNKNOWN]  = */[](sqlite3_value *sv, value &v) { },
            /*[INT]      = */[](sqlite3_value *sv, value &v) { v = sqlite3_value_int(sv); },
            /*[VARCHAR]  = */[](sqlite3_value *sv, value &v) {
                auto str = reinterpret_cast<const char *>(sqlite3_value_text(sv));
                auto len =  strlen(str)+1;
                v.ptr = new char[len];
                memcpy(v.ptr, str, len); },
            /*[CHAR]  = */[](sqlite3_value *sv, value &v) {
                auto str = reinterpret_cast<const char *>(sqlite3_value_text(sv));
                auto len =  strlen(str)+1;
                v.ptr = new char[len];
                memcpy(v.ptr, str, len); },
            /*[BIGINT]   = */[](sqlite3_value *sv, value &v) { v = sqlite3_value_int64(sv); },
            /*[BOOLEAN]  = */[](sqlite3_value *sv, value &v) { v = sqlite3_value_int(sv); },
            /*[DOUBLE]  = */[](sqlite3_value *sv, value &v) { v = sqlite3_value_double(sv); },
            /*[SMALLINT] = */[](sqlite3_value *sv, value &v) { v = sqlite3_value_int(sv); },
            /*[TINYINT]  = */[](sqlite3_value *sv, value &v) { v = sqlite3_value_int(sv); },
    };
    mapping[type](sv,v);
}

parser::parser(table &table) : belong_to(table) { }


// TODO 提高这个函数的性能和代码可读性
record parser::raw2rec(const char *raw, size_t raw_size) {
    auto &schema = belong_to.get_schema();
    auto &cols = schema.get_columns();
    auto offset = schema.get_offset();
    auto cur_offset = 0;
    std::vector<value> values;
    std::vector<record::var_ptr> var_info;
    column_id_t cid = 0;
    for(const auto &col : cols) {
        auto type = col.get_type();
        value v { };
        if(type == col_type::CHAR || type == col_type::VARCHAR) {
            auto str = raw + offset;
            auto info = reinterpret_cast<const uint32_t*>(raw + cur_offset);
            auto off = info[0];
            auto len = info[1]; // 带有结束符位置了
            debug_only(
                std::cout << "in raw2rec: [off] = " << off << " [len] = " << len << std::endl;
            );
            offset += len;
            v.ptr = new char[len];
            memcpy(v.ptr, str, len);
            debug_only(
                std::cout << "in raw2rec: [pointer] = " << v.ptr << std::endl; // OK
            );
            var_info.emplace_back(cid, off, len);
        } else {
            if(type == col_type::DOUBLE) v.f64 = *reinterpret_cast<const double*>(raw + cur_offset);
            else if(type == col_type::BIGINT) v.i64 = *reinterpret_cast<const int64_t*>(raw + cur_offset);
            else v.i32 = *reinterpret_cast<const int32_t*>(raw + cur_offset);
            // TODO 添加更多类型支持
        }
        cur_offset += col_type_handler::size(type);
        ++cid;
        values.push_back(v);
    }
    return record(values, var_info, raw_size, true); // TODO 这种诡异写法有空改改，如果换成move就不用这样了
}

size_t parser::rec2raw(const record &rec, char *&raw) { // 感觉没错
    auto &schema = belong_to.get_schema();
    auto &cols = schema.get_columns();
    // assert cols.size() == rec.size()
    auto n = cols.size();
    auto cur_offset = 0; // 类型的占位字节数
    size_t n_byte = schema.get_offset(); // 全局
    raw = new char[rec.get_raw_size()];
    for(auto i = 0, j = 0; i < n; ++i) {
        auto &col = cols[i];
        auto type = col.get_type();
        auto v = rec.get(i);
        if(type == col_type::CHAR || type == col_type::VARCHAR) {
            auto &var_info = rec.get_var(j++);
            auto off = std::get<1>(var_info);
            auto len = std::get<2>(var_info);
            auto start = reinterpret_cast<uint32_t*>(raw + cur_offset);
            start[0] = off;
            start[1] = len;
            memcpy(raw + off, v.ptr, len);
            debug_only(
                std::cout << "[raw]: {" << (char*)(raw+off) << "}" << std::endl; // OK
            );
            n_byte += len;
        } else {
            if(type == col_type::DOUBLE) *reinterpret_cast<double*>(raw + cur_offset) = v.f64;
            else if(type == col_type::BIGINT) *reinterpret_cast<int64_t*>(raw + cur_offset) = v.i64;
            else *reinterpret_cast<int32_t*>(raw + cur_offset) = v.i32;
        }
        auto size = col_type_handler::size(type);
        // n_byte += size;
        cur_offset += size;
    }
    return n_byte;
}
