#ifndef VSQL_TX_H
#define VSQL_TX_H

#include <map>
#include <unordered_map>
#include <memory>
#include "../defs/defs.h"
#include "log_obj.h"
#include "../page/page.h"


// transaction
// 描述简单事务的内部状态
// 状态记录粒度是页级
class tx {

    // 虽然map最坏复杂度比哈希差
    // 但是在单个事务涉及的页数少于1000时有足够的性能优势

    tx_id_t tx_id{}; // 事务的id随时间分配
    bool visit{};
    std::unordered_map<page_id_t, log_obj> before;   // TODO 已经用不着了 删了
    std::unordered_map<page_id_t, log_obj> after; // after总是最近状态

    std::unordered_map<page_id_t, std::unique_ptr<page>> old_p_data; // 用于undo，保证是事务做出任何修改前的版本
    std::unordered_map<page_id_t, std::unique_ptr<page>> p_data; // transaction private data
public:
    tx() = default;
    explicit tx(tx_id_t tx_id);
    bool visited(page_id_t page_id) const;
    void first_log(page_id_t page_id, const page& val); // 用于undo // TODO 删了
    void last_log(page_id_t page_id, const page& val);  // 用于redo // TODO 删
    bool operator < (const tx &that) const; // 用于map中按txid排序

    bool has_page(page_id_t page_id) {
        return p_data.find(page_id) != p_data.end();
    }

    template<typename page_type>
    void add_page(page_id_t page_id, const page_type &page_copy) {
        p_data.emplace(page_id, std::unique_ptr<page>(new page_type(page_copy)));
    }

    template<typename page_type>
    page_type& get_page(page_id_t page_id) {
        return static_cast<page_type&>(*p_data[page_id]);
    }

    bool has_backup(page_id_t page_id) {
        return old_p_data.find(page_id) != old_p_data.end();
    }

    // 做出undo备份，【默认该页必然存在】
    template<typename page_type>
    page_type& backup(page_id_t page_id) {
        auto &old_page = get_page<page_type>(page_id);
        old_p_data.emplace(page_id, std::unique_ptr<page>(new page_type(old_page)));
        return old_page;
    }

    std::unordered_map<page_id_t, std::unique_ptr<page>>&
    get_redo_map() {
        return p_data;
    }

    tx_id_t get_tx_id() { return tx_id; }

};

#endif
