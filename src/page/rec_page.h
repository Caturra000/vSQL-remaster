#ifndef VSQL_REC_PAGE_H
#define VSQL_REC_PAGE_H


#include "page.h"

/*
 * 大概长这样
              ↑→-------------------------------------↓
              ↑                                      ↓
 ----------------------------------------------------·----------------------------------
| MAGIC | END_FREE | PTR_CNT | PTR[0] | PTR[1] | .. | REC[N-1] | REC[N-2] | .. | REC[0] |
 -------------------------------------------------------------------------------·-------
                      = N        ↓                                              ↑
|←          FIXED           →|   ↓→---------------------------------------------↑


 而REC[i]的内部大概是这样
  ------------------------------------------------------------
| (998244) | (28,7) | (10^12+7) | (35,5) | "kiseki" | "jojo" |
 ·----------·--------·-----------·--------·----------·-------
 0          4        12          20       28         35
|←                FIXED                 →|

 */


// 采用slotted-page机制的记录页
// 空间利用率：
// 理论最坏情况1字节 ≈20%
// 实际极端8字节 ≈66%
// 常规记录32字节大小， ≈81%
// 2000字节大型记录， ≈97%
class rec_page : public page {
protected:
    void swap(rec_page &that);
public:

    explicit rec_page(uint32_t magic = RECORD);
    rec_page(const rec_page &that);
    rec_page(rec_page &&that) noexcept;
    explicit rec_page(char *&&temp_data);
    rec_page& operator = (const rec_page &that);
    rec_page& operator = (rec_page &&that) noexcept;

    enum magic_type {
        RECORD = 0x1000,
    };

    accessor(LSN,       uint32_t,  field_end_magic); // 事务 X 不干了
    accessor(next_page, page_id_t, field_end_LSN);  // 溢出页X 不干了
    accessor(end_free,  offset_t,  field_end_next_page);
    accessor(ptr_cnt,   uint32_t,  field_end_end_free);
    static constexpr size_t end_fixed = field_end_ptr_cnt;
private:
    offset_t *ptr = reinterpret_cast<offset_t*>(data + end_fixed); // 指针指向页内偏移量
public:
    bool can_store(size_t n_byte) const;
    void add_ptr(offset_t offset);
    void add_rec(void *rec, size_t n_byte);
    offset_t get_rec_size(size_t pos) const;
    const char* get_rec(int pos) const; // 返回的是记录的绝对地址
    page_off_t get_ptr(int pos) const; // 返回ptr的值（段内偏移）

};








#endif //VSQL_REC_PAGE_H
