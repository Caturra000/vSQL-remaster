#ifndef VSQL_VTABLE_H
#define VSQL_VTABLE_H

#include "../sqlite/sqlite3.h"

// 创建虚表
// 需要解析： argv[0] module, argv[1] database, argv[2] table, argv[3] columns, argv[4] keys
int vtab_create(sqlite3 *db, void *pAux, int argc, const char *const *argv,
                sqlite3_vtab **ppVtab, char **pzErr);

// 解析同理
int vtab_connect(sqlite3 *db, void *pAux, int argc, const char *const *argv,
                 sqlite3_vtab **ppVtab, char **pzErr);

// 需要解析idxInfo
int vtab_best_index(sqlite3_vtab *tab, sqlite3_index_info *pIdxInfo);

int vtab_disconnect(sqlite3_vtab *pVtab);

int vtab_open(sqlite3_vtab *pVtab, sqlite3_vtab_cursor **ppCursor);

int vtab_close(sqlite3_vtab_cursor *cur);


int vtab_filter(sqlite3_vtab_cursor *pVtabCursor, int idxNum, const char *idxStr,
                int argc, sqlite3_value **argv);

int vtab_next(sqlite3_vtab_cursor *cur);

int vtab_eof(sqlite3_vtab_cursor *cur);

int vtab_column(sqlite3_vtab_cursor *cur, sqlite3_context *ctx, int i);

int vtab_rowid(sqlite3_vtab_cursor *cur, sqlite3_int64 *pRowid);

int vtab_update(sqlite3_vtab *pVTab, int argc, sqlite3_value ** argv,
                sqlite3_int64 *pRowid);

int vtab_begin(sqlite3_vtab *pVTab);

int vtab_commit(sqlite3_vtab *pVTab);

#endif //VSQL_VTABLE_H

