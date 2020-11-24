#ifndef VSQL_CURSOR_H
#define VSQL_CURSOR_H

#include "table.h"
#include "constrain.h"
#include "iterator.h"

class cursor {
    sqlite3_vtab_cursor vcur { };
    table &belong_to;
    iterator iter;
public:
    table& get_table();
    iterator& get_iter();
    void query();
//    void add();
//    void remove();
//    void update();
    void rewind();
    void next();
    bool eof();
    explicit cursor(table &table);

};

#endif //VSQL_CURSOR_H
