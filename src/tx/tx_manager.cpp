#include "tx_manager.h"

tx &tx_manager::get_tx(tx_id_t tx_id) {
    return mapping[tx_id];
}

tx_manager::tx_manager(const std::string &file_name): tx_disk(file_name) { }

tx_manager::tx_manager(): tx_disk("null") {

}

int tx_manager::next_tx_id() {
    return ++counter;
}

tx_io &tx_manager::get_tx_io() {
    return tx_disk;
}

void tx_manager::remove_tx(tx_id_t tx_id) {
    mapping.erase(tx_id);
}
