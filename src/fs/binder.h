#ifndef VSQL_BINDER_H
#define VSQL_BINDER_H

#include <map>
#include "free_manager.h"
#include "io_manager.h"
#include "rec_manager.h"

// 专门拯救我的垃圾代码的类
class binder {
//    std::map<std::string, free_manager> fm_map;
//    std::map<std::string, io_manager> io_map;
//    std::map<std::string, rec_manager> rm_map; // 我本以为static就稳了


    // 由于个人g++版本比较旧，使用map偶尔不支持???

    std::unordered_map<std::string, std::unique_ptr<free_manager>> fm_map;
    std::unordered_map<std::string, std::unique_ptr<io_manager>> io_map;
    std::unordered_map<std::string, std::unique_ptr<rec_manager>> rm_map;
    std::unordered_map<std::string, std::unique_ptr<tx_manager>> tm_map;


    binder() = default;
public:
    static binder& get_binder() {
        static binder b;
        return b;
    }

    void create(const std::string &file_name) {
        if(fm_map.find(file_name) != fm_map.end()) return;
        auto &h = header_factory::get_factory().get_header(file_name);
        fm_map.emplace(file_name, std::unique_ptr<free_manager>(new free_manager(file_name, h)));
        tm_map.emplace(file_name, std::unique_ptr<tx_manager>(new tx_manager(file_name)));
        io_map.emplace(file_name, std::unique_ptr<io_manager>(new io_manager(file_name, *fm_map[file_name], *tm_map[file_name])));
        rm_map.emplace(file_name, std::unique_ptr<rec_manager>(new rec_manager(*io_map[file_name], h)));
    }

    free_manager& get_fm(const std::string &file_name) {
        if(fm_map.find(file_name) == fm_map.end()) create(file_name);
        return *fm_map[file_name];
    }

    io_manager& get_io(const std::string &file_name) {
        if(io_map.find(file_name) == io_map.end()) create(file_name);
        return *io_map[file_name];
    }

    rec_manager& get_rm(const std::string &file_name) {
        if(rm_map.find(file_name) == rm_map.end()) create(file_name);
        return *rm_map[file_name];
    }

    tx_manager& get_tm(const std::string &file_name) {
        if(tm_map.find(file_name) == tm_map.end()) create(file_name);
        return *tm_map[file_name];
    }

    binder(const binder&) = delete;
    binder& operator = (const binder&) = delete;

};


#endif //VSQL_BINDER_H
