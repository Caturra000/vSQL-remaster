#include "header_factory.h"

header_factory &header_factory::get_factory() {
    static header_factory factory;
    return factory;
}

head_page &header_factory::get_header(const std::string &file_name) {
    if(mapping.count(file_name)) {
        return mapping[file_name];
    } else {
        head_page h;
        disk(file_name).read(0,h);
        if(h.get_magic() == page::UNDEFINED) {
            h = head_page();
        }
        mapping[file_name] = h;
        return mapping[file_name];
    }
}

header_factory::~header_factory() {
    for(auto it : mapping) {
        //   name               content
        disk(it.first).write(0, it.second);
    }
}
