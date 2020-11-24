#include "column.h"

#include <utility>

column::column(column_id_t column_id, column_id_t offset,
               std::string column_name, col_type type)
    : column_id(column_id), offset(offset), column_name(std::move(column_name)),
      type(type) { }

col_type column::get_type() const {
    return type;
}

