#include <cstring>
#include "table.h"
#include "value.h"
#include "record.h"


//table::table(const std::string &table_name, const std::string &column_args)
//        : _schema(table_name, column_args), _fs(table_name + ".table") { }
//
//table::table(const std::string &table_name, const std::string &column_args, const std::string &key_args)
//        : _schema(table_name, column_args, key_args), _fs(table_name + ".table") { }

schema &table::get_schema() {
    return _schema;
}

fs &table::get_fs() {
    return _fs;
}

void table::remove(int64_t key) {
    auto &index = _fs.get_index();
    index.remove(key);
}

table::table(const std::string &table_name, const std::vector<std::string> &args)
        : _schema(table_name, args), _fs(table_name + ".table") { }
