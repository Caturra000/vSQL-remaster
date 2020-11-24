#ifndef VSQL_COLUMN_H
#define VSQL_COLUMN_H

#include <vector>
#include "../defs/defs.h"
#include "type.h"

class column {
    column_id_t column_id;
    column_id_t offset; // 暂时没啥用处
    std::string column_name;
    col_type    type;
    // int64_t restrict; // ==type.size if no restrict // CURRENTLY DO NOT SUPPORT RESTRICT
    //std::vector<std::string> overflow; // for varchar // overflow[rowid] = var
public:
    column(column_id_t column_id, column_id_t offset, std::string column_name, col_type type);
    col_type get_type() const;
};

#endif //VSQL_COLUMN_H
