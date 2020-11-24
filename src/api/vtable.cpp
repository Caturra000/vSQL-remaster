#include <cassert>
#include <cstring>
#include <algorithm>
#include <mutex>
#include <atomic>
#include "vtable.h"
#include "../table/table.h"
#include "../table/cursor.h"
#include "../table/parser.h"
#include "../sqlite/sqlite3ext.h"
#include "../utils/guard.h"

SQLITE_EXTENSION_INIT1



// argv[2]: table_name
// argv[3]: columns
// argv[4]: key_attrs

// UPDATE. 考虑改一下样式变为 argv[3...]都是分别一个column，这样我连解析都省了，而且形式更加自然
// 这样前两个肯定是name type的形式，而第三个才是key，这是考虑是P还是K开头就可以了
int vtab_create(sqlite3 *db, void *pAux, int argc,const char *const *argv,
                sqlite3_vtab **ppVtab, char **pzErr) {
    assert(argc >= 4);
    std::string declare = "CREATE TABLE X(";
    for(auto i = 3; i < argc; ++i) {
        declare += argv[i];
        if(i != argc-1) declare += ",";
        else declare += ");";
    }
    auto rc = sqlite3_declare_vtab(db, declare.c_str());
    if(rc != SQLITE_OK) return rc;
    std::string table_name = argv[2];
    std::transform(table_name.begin(), table_name.end(),
            table_name.begin(), [](char c) { return std::tolower(c); });
    std::vector<std::string> columns;
    for(auto i = 3; i < argc; ++i) {
        auto arg = string_utils::trim(argv[i]);
        std::transform(arg.begin(), arg.end(), arg.begin(),
                [](char c) { return std::tolower(c); });
        columns.emplace_back(arg);
    }
    auto tab = new table(table_name, columns);
    *ppVtab = reinterpret_cast<sqlite3_vtab*>(tab);
    // TODO header中添加table信息
    tab->get_fs().recover();
    return SQLITE_OK;
}

int vtab_connect(sqlite3 *db, void *pAux, int argc, const char *const *argv,
                 sqlite3_vtab **ppVtab, char **pzErr) {
    return vtab_create(db, pAux, argc, argv, ppVtab, pzErr);
}

// omit和double check需要再看一下
// update.这里只负责找主键就算了，合不合法交给上层应用处理
// 更准确点是这里只负责判断有没有踩中索引？
int vtab_best_index(sqlite3_vtab *vtab, sqlite3_index_info *pIdxInfo) {
    auto n = pIdxInfo->nConstraint;
    auto str = pIdxInfo->idxStr = new char[n+1];
    pIdxInfo->needToFreeIdxStr = false; // 自己控制
    auto cnt = 0;
    auto &tab = *reinterpret_cast<table*>(vtab);
    auto idx_pos = tab.get_schema().get_primary();
    for(int i = 0; i < n; ++i) { // 暂不支持辅助索引
        if(!pIdxInfo->aConstraint[i].usable) continue;
        auto col = pIdxInfo->aConstraint[i].iColumn;
        if(col != idx_pos) continue; // 非主键约束给sqlite帮我搞
        auto op = pIdxInfo->aConstraint[i].op;
        if(op > 32) continue; // 索引不支持 != 及更多奇怪的约束
        pIdxInfo->aConstraintUsage[i].argvIndex = cnt+1;
        str[cnt++] = op; // 范围 <= 32
    }
    if(cnt) pIdxInfo->idxNum = 1; // 可用主键
    // if(tab.get_schema().primary_hidden()) pIdxInfo->idxNum = 1;
    return SQLITE_OK;
}

int vtab_disconnect(sqlite3_vtab *pVtab) {
    auto tab = reinterpret_cast<table*>(pVtab);
    delete tab;
    return SQLITE_OK;
}

int vtab_open(sqlite3_vtab *pVtab, sqlite3_vtab_cursor **ppCursor) {
    auto tab = reinterpret_cast<table*>(pVtab);
    auto cur = new cursor(*tab);
    *ppCursor = reinterpret_cast<sqlite3_vtab_cursor*>(cur);
    return SQLITE_OK;
}

int vtab_close(sqlite3_vtab_cursor *cur) {
    delete reinterpret_cast<cursor*>(cur);
    return SQLITE_OK;
}

// 其实argv只保留索引部分就可以了
int vtab_filter(sqlite3_vtab_cursor *pVtabCursor, int idxNum,
                const char *idxStr, int argc, sqlite3_value **argv) {
    auto &cur = *reinterpret_cast<cursor*>(pVtabCursor);
    constrain<pri_key_t> cons; // 默认约束是全部合法
    if(idxNum && argc) {
        for(auto i = 0; i < argc; ++i) {
            auto op = idxStr[i];
            auto v = sqlite3_value_int64(argv[i]);
            cons = cons & constrain<pri_key_t>(op, v);
        }
    }

    debug_only(
        std::cout << "[constrain] = ";
        cons.out();
        std::cout << std::endl;
    );

    cur.get_iter().set_constrain(cons);
    cur.query();
    cur.rewind();
    delete[] idxStr;
    return SQLITE_OK;
}

int vtab_next(sqlite3_vtab_cursor *vcur) {
    reinterpret_cast<cursor*>(vcur)->next();
    return SQLITE_OK;
}

int vtab_eof(sqlite3_vtab_cursor *vcur) {
    return reinterpret_cast<cursor *>(vcur)->eof();
}

int vtab_column(sqlite3_vtab_cursor *vcur, sqlite3_context *ctx, int i) {
    auto &cur = *reinterpret_cast<cursor*>(vcur);
    const auto &record = cur.get_iter().cur_record();
    auto val = record.get(i);
    auto type = cur.get_table().get_schema().get_column(i).get_type();
    parser::decode(ctx, type, val); // 避免switch case
    return SQLITE_OK;
}

int vtab_rowid(sqlite3_vtab_cursor *vcur, sqlite3_int64 *pRowid) {
    auto &cur = (*reinterpret_cast<cursor*>(vcur));
    auto &rec = cur.get_iter().cur_record();
    auto pri = cur.get_table().get_schema().get_primary();
    *pRowid = rec.get(pri).i64;
    // *pRowid = (*reinterpret_cast<cursor*>(vcur)).get_iter().cur_record().get_rowid();
    return SQLITE_OK;
}

int vtab_update(sqlite3_vtab *pVTab, int argc, sqlite3_value **argv, sqlite3_int64 *pRowid) {

    // DELETE
    // 主键是argv[0]
    auto &tab = *reinterpret_cast<table*>(pVTab);
    auto &index = tab.get_fs().get_index();
    if(argc == 1) {
        if(sqlite3_value_type(argv[0]) == SQLITE_NULL) return SQLITE_OK;
        index.remove(sqlite3_value_int64(argv[0])); // 从rowid获得的值

        // argc == n+2
        // INSERT
        // 从argv[2]开始
        // argv[1]作为主键，如果为空，就要自动生成，空时需要*pRowid=新的。。。以告诉sqlite相关信息
        // 只有 argc > 1 && argv[1] == null 才会有效设置pRowid
        // 实测INSERT时 argv[0] 和 argv[1]都为空
    } else if(sqlite3_value_type(argv[0]) == SQLITE_NULL) {
        auto &schema = tab.get_schema();
        auto &cols = schema.get_columns();
        auto par = parser(tab);
        std::vector<value> values;
        std::vector<record::var_ptr> var_info;
        auto offset = schema.get_offset();
        auto hidden = schema.primary_hidden();
        auto size = cols.size() - hidden;
        long long hidden_rowid = 0;
        for(auto i = 0; i < size; ++i) {
            auto &col = cols[i];
            auto type = col.get_type();
            value v {};
            parser::encode(argv[i+2],type,v);
            values.push_back(v);
            if(type == CHAR || type == VARCHAR) { // 感觉OK?
                auto str = reinterpret_cast<const char*>(sqlite3_value_text(argv[i+2]));
                auto len = strlen(str) + 1; // \n
                auto info = record::make_var_ptr(i, offset, len);
                offset += len;
                var_info.push_back(info);
            }
        }
        if(hidden) {
            auto type = cols[size].get_type(); // size为隐藏列所在cid
            value v {};
            if(type == col_type::BIGINT) {
                v.i64 = hidden_rowid = schema.generate_id();
            } else {
                v.i32 = hidden_rowid = schema.generate_id();
            }
            values.push_back(v);
        }
        auto rec = record(values, var_info, offset);
        char *raw = nullptr;
        auto g = guard::make([&]() { delete[] raw; });
        auto n_byte = par.rec2raw(rec, raw);
        index.add(rec.get(schema.get_primary()).i64, raw, n_byte);
        // delete[] raw;

        // pRowid处理
        if(hidden) {
            *pRowid = hidden_rowid;
        } else if(col_type_handler::size(cols[schema.get_primary()].get_type()) == 4) {
            *pRowid = sqlite3_value_int(argv[2 + schema.get_primary()]);
        } else {
            *pRowid = sqlite3_value_int64(argv[2 + schema.get_primary()]);
        }

        // UPDATE
        // rowid/pk 为argv[0](==argv[1]) 被更新为argv[2] argv[3].... 就是键不变的意思吧
        // 如果0和1不等，就是0 / rowid 被替换为1 / rowid 。。 在sqlite层面是 set rowid = xx
    } else {
        // 直接复制过来了

        index.remove(sqlite3_value_int64(argv[0]));

        auto &schema = tab.get_schema();
        auto &cols = schema.get_columns();
        auto par = parser(tab);
        std::vector<value> values;
        std::vector<record::var_ptr> var_info;
        auto offset = schema.get_offset();
        auto hidden = schema.primary_hidden();
        auto size = cols.size() - hidden;
        for(auto i = 0; i < size; ++i) {
            auto &col = cols[i];
            auto type = col.get_type();
            value v {};
            parser::encode(argv[i+2],type,v);
            values.push_back(v);
            if(type == CHAR || type == VARCHAR) {
                auto str = reinterpret_cast<const char*>(sqlite3_value_text(argv[i+2]));
                auto len = strlen(str) + 1; // \n
                auto info = record::make_var_ptr(i, offset, len);
                offset += len;
                var_info.push_back(info);
            }
        }
        if(hidden) { // 如果是hidden，那么update肯定没办法显式修改这个值 生成新的key也可以把
            auto type = cols[size].get_type(); // size为隐藏列所在cid
            value v { };
            v = schema.generate_id();
            values.push_back(v);
        }
        auto rec = record(values, var_info, offset);
        char *raw = nullptr;
        auto g = guard::make([&]() { delete[] raw; });
        auto n_byte = par.rec2raw(rec, raw);
        index.add(rec.get(schema.get_primary()).i64, raw, n_byte);
        // delete[] raw;

    }
    return SQLITE_OK;
}

std::mutex mtx;
std::atomic<bool> has_mtx;

int vtab_begin(sqlite3_vtab *pVTab) {
    mtx.lock();
    has_mtx = true;
    auto tab = reinterpret_cast<table*>(pVTab);
    tab->get_fs().start_transaction();
    debug_only(
        std::cout << "begin transaction" << std::endl;
    );
    return SQLITE_OK;
}

int vtab_commit(sqlite3_vtab *pVTab) {
    auto tab = reinterpret_cast<table*>(pVTab);
    tab->get_fs().commit();
    debug_only(
        std::cout << "commit" << std::endl;
    );
    if(has_mtx) { // 可能不经过begin直接执行commit,比如简单的建表
        has_mtx = false;
        mtx.unlock();
    }
    return SQLITE_OK;
}

sqlite3_module vtable_module = {
        0,                  /* iVersion */
        vtab_create,        /* xCreate */
        vtab_connect,       /* xConnect */
        vtab_best_index,    /* xBestIndex */
        vtab_disconnect,    /* xDisconnect */
        vtab_disconnect,    /* xDestroy */
        vtab_open,          /* xOpen - open a cursor */
        vtab_close,         /* xClose - close a cursor */
        vtab_filter,        /* xFilter - configure scan constraints */
        vtab_next,          /* xNext - advance a cursor */
        vtab_eof,           /* xEof - check for end of scan */
        vtab_column,        /* xColumn - read data */
        vtab_rowid,         /* xRowid - read data */
        vtab_update,        /* xUpdate */
        vtab_begin,         /* xBegin */
        0,                  /* xSync */
        vtab_commit,        /* xCommit */
        0,                  /* xRollback */
        0,                  /* xFindMethod */
        0,                  /* xRename */
        0,                  /* xSavepoint */
        0,                  /* xRelease */
        0,                  /* xRollbackTo */
};



#ifdef _WIN32
__declspec(dllexport)
#endif
extern "C"
int sqlite3_vtable_init(sqlite3 *db, char **pzErrMsg,
                        const sqlite3_api_routines *pApi) {
    SQLITE_EXTENSION_INIT2(pApi);
    return sqlite3_create_module(db, "vtable", &vtable_module, nullptr);
}
