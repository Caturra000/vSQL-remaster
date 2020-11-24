#include "iterator.h"

void iterator::set_constrain(const constrain<int64_t> &status) {
    index_status = status;
}


bool iterator::eof() {
    return offset >= result.size();
}

void iterator::next() {
    ++offset;
}

const record &iterator::cur_record() const {
    return result[offset];
}

const constrain<int64_t> &iterator::idx_stat() const {
    return index_status;
}


// iterator::iterator(cursor &c) : belong_to(c), offset(0) { }

void iterator::add_rec(const record &rec) {
    result.push_back(rec);
}

void iterator::begin() {
    offset = 0;
}

void iterator::clear() {
    result.clear();
}

