#ifndef VSQL_ITERATOR_H
#define VSQL_ITERATOR_H

#include <tuple>
#include <vector>
#include "../defs/defs.h"
#include "value.h"
#include "constrain.h"
#include "record.h"


class iterator {
    constrain<pri_key_t>    index_status;
    std::vector<record>     result;
    int                     offset = 0; // 将要读取的行

public:
    void set_constrain(const constrain<int64_t> &status);

    void begin();
    bool eof();
    void next();
    void clear();
    const record& cur_record() const;
    const constrain<int64_t>& idx_stat() const;
    void add_rec(const record &rec);

};

#endif //VSQL_ITERATOR_H
