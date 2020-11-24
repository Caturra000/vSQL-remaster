#ifndef VSQL_DISK_H
#define VSQL_DISK_H

#include "../defs/defs.h"
#include "../page/page.h"


class disk {
    const std::string file_name;
    const std::string open_mode;
    FILE *file; // 由于fstream过于臃肿，还是用FILE了

public:
    explicit disk(const std::string &file_name) : file_name(file_name), open_mode("rb+") {
        file = fopen(file_name.c_str(), open_mode.c_str());
        if (!file) {
            file = fopen(file_name.c_str(), "wb");
            fclose(file);
            file = fopen(file_name.c_str(), open_mode.c_str());
        }
    }

    ~disk() { fclose(file); }

    void write(page_id_t page_id, page &page) {
        fseek(file, page_id * PAGE_SIZE, SEEK_SET);
        fwrite(page.get_data(), PAGE_SIZE, 1, file); // OK
    }

    // 虽然不得已用了template，但还是足够安全的
    // template <typename page_type>
    void read(page_id_t page_id, page &page) {
        // static_assert(std::is_base_of<class page, page_type>::value, "must be a page");
        fseek(file, page_id * PAGE_SIZE, SEEK_SET);
        page.set_magic(page::UNDEFINED);
        page.overlap_from(file); // 只需8K
//        char *xvalue = new char[PAGE_SIZE];
//        *(reinterpret_cast<uint32_t*>(xvalue)) = page::UNDEFINED; // 避免写入失败，如果失败那最后的page也是UNDEFINED
//        fread(xvalue, PAGE_SIZE, 1, file);
//        page = page_type(std::move(xvalue)); // 虽然有移动语义，但还是得花费16KB
    }
};



#endif //VSQL_DISK_H
