#ifndef VSQL_TX_MANAGER_H
#define VSQL_TX_MANAGER_H

#include "tx.h"
#include "tx_io.h"

// 作为tx模块的顶级接口嵌入到fs模块中
class tx_manager {
    int counter { }; // 后期需要线程安全
    tx_io tx_disk;
    std::unordered_map<tx_id_t, tx> mapping;
public:
    tx_manager();
    explicit tx_manager(const std::string &file_name);
    tx& get_tx(tx_id_t tx_id);
    int next_tx_id();
    static tx_manager& null() {
        static tx_manager null_obj;
        return null_obj;
    }
    tx_io& get_tx_io(); // 偷懒一下
    void remove_tx(tx_id_t tx_id);
};
#endif //VSQL_TX_MANAGER_H
