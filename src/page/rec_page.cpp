#include "rec_page.h"

rec_page::rec_page(uint32_t magic) : page(magic) {
    set_LSN(0);
    set_next_page(0);
    set_end_free(PAGE_SIZE);
    set_ptr_cnt(0);
}

rec_page::rec_page(char *&&temp_data) : page(std::move(temp_data)) { }

rec_page &rec_page::operator=(const rec_page &that) {
    if(this == &that) return *this;
    if(that.is_fast_copy()) {
        memcpy(data, that.data, end_fixed);
    } else {
        memcpy(data, that.data, PAGE_SIZE);
    }
    return *this;
}

bool rec_page::can_store(size_t n_byte) const {
    auto left_space = field_end_ptr_cnt + sizeof(offset_t) * get_ptr_cnt(); // 左边
//  return get_end_free() - n_byte - sizeof(offset_t) >= left_space; // end_free往左走会不会触发
    return get_end_free() > left_space + n_byte + sizeof(offset_t); // 避免无符号负数溢出
}

void rec_page::add_ptr(offset_t offset) {
    auto cnt = get_ptr_cnt();
    ptr[cnt] = offset;
    set_ptr_cnt(cnt+1);
}

void rec_page::add_rec(void *rec, size_t n_byte) {
    auto end_free = get_end_free();
    auto start_rec = end_free-n_byte;
    memmove(data+start_rec, rec, n_byte);
    add_ptr(start_rec);
    set_end_free(start_rec);
}

offset_t rec_page::get_rec_size(size_t pos) const {
    if(pos == 0) return PAGE_SIZE - ptr[0];
    return ptr[pos-1]-ptr[pos];
}

const char *rec_page::get_rec(int pos) const {
    return get_data()+ptr[pos];
}

page_off_t rec_page::get_ptr(int pos) const {
    return ptr[pos];
}

void rec_page::swap(rec_page &that) {
    page::swap(that);
    std::swap(ptr, that.ptr);
}

rec_page::rec_page(const rec_page &that): page(that) { }

rec_page::rec_page(rec_page &&that) noexcept: page(std::move(that)), ptr(that.ptr) {
    that.ptr = nullptr;
}

rec_page &rec_page::operator=(rec_page &&that) noexcept {
    this->swap(that);
    return *this;
}


// 废弃的函数
//    void delete_rec(int pos) {
//        auto cnt = get_ptr_cnt();
//        if(pos < 0 || pos >= cnt) return;
//        auto size = get_rec_size(pos);
//        auto off = get_end_free(); // end_free是数据开端
//
//        // 移动数据
//        if(pos != cnt-1) {
//            memmove(data + off + size,
//                    data + off,
//                    ptr[pos] - off);
//        }
//
//        // 移动指针
//        set_ptr_cnt(--cnt);
//        for(auto i = pos; i < cnt; ++i) {
//            ptr[i] = ptr[i+1] + size;
//        }
//        set_end_free(get_end_free()+size);
//    }