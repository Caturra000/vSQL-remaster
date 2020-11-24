//#ifndef VSQL_FAST_IO_H
//#define VSQL_FAST_IO_H
//
//#include "io_manager.h"
//
//// IO块存在的问题是即使是缓存也要经过冗余的命中LRU和访问index mapping的过程
//// 因此在连续调用多次的同一页时依然会不必要地降低性能
//// 为了避免这个问题多装饰一层更为单调的读、写缓存
//// 由于是只保持单一的热点数据，不会产生一致性问题（突然被淘汰）

//template<typename page_type>
//class fast_io {
//    io_manager<page_type> io_manager;
//    page_type *write_eden;
//    const page_type *read_eden;
//    page_id_t eden_id;
//    bool read; // 表明当前是读缓存还是写缓存，避免从读缓存中获得写缓存
//public:
//    fast_io(const std::string &file_name, free_manager &fm);
//    const page_type& read_op(page_id_t page_id); // 不进行判空处理
//    page_type& write_op(page_id_t page_id); // 不进行判空处理
//    void clear_eden(); // 总有可能会被主动干掉
//    page_id_t alloc(uint32_t magic = page::UNDEFINED);
//    void dealloc(page_id_t page_id);
//};
//
//template<typename page_type>
//fast_io<page_type>::fast_io(const std::string &file_name, free_manager &fm)
//        : io_manager(file_name,fm), write_eden(nullptr), read_eden(nullptr),
//            eden_id(0), read(false) { }
//
//// 优化：同一个id，写了之后再读 和 读了之后再写 相比，前者可直接转移
//template<typename page_type>
//const page_type &fast_io<page_type>::read_op(page_id_t page_id) {
//    if(page_id == eden_id) {
//        if(read) return *read_eden;
//        else return *write_eden;  // 小优化，已经标记过脏页了
//    }
//    read = true;
//    eden_id = page_id;
//    read_eden = &io_manager.read_op(page_id);
//    return *read_eden;
//}
//
//template<typename page_type>
//page_type &fast_io<page_type>::write_op(page_id_t page_id) {
//    if(!read && page_id == eden_id) {
//        return *write_eden;
//    }
//    read = false;
//    eden_id = page_id;
//    write_eden = &io_manager.write_op(page_id);
//    return *write_eden;
//}
//
//template<typename page_type>
//void fast_io<page_type>::clear_eden() {
//    read_eden = nullptr;
//    write_eden = nullptr;
//    eden_id = 0;
//    read = false;
//}
//
//template<typename page_type>
//page_id_t fast_io<page_type>::alloc(uint32_t magic) {
//    clear_eden();
//    return io_manager.alloc(magic);
//}
//
//template<typename page_type>
//void fast_io<page_type>::dealloc(page_id_t page_id) {
//    clear_eden();
//    io_manager.dealloc(page_id);
//}
//
//#endif //VSQL_FAST_IO_H
