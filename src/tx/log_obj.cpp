#include "log_obj.h"

log_obj::log_obj(const page &raw) : backup(new char[PAGE_SIZE]) {
    auto raw_data = raw.get_data();
    memcpy(backup, raw_data, PAGE_SIZE);
}

log_obj::log_obj() : backup(nullptr) { }
