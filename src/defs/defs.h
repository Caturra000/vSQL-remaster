#ifndef VSQL_DEFS_H
#define VSQL_DEFS_H


#include <cstdint>
#include <string>
#include <limits>
#include <ctime>


// everything about configuration


using row_id_t = std::uint32_t; // row id

using page_id_t = uint32_t;
const size_t PAGE_SIZE = 8192;
const size_t PAGE_MAX = (page_id_t)-1;

using column_id_t = uint32_t; // sqlite最多支持到int64
const size_t COLUMN_MAX = std::numeric_limits<column_id_t>::max();  //(column_id_t)-1;

using offset_t = uint32_t;
using page_off_t = uint64_t;

using pri_key_t = int64_t;

using tx_id_t = uint32_t;

// 时间戳类型， 注意这个在C++20前都不保证跨平台epoch一致
using ts_t = decltype(std::time(nullptr));


#endif //VSQL_DEFS_H

