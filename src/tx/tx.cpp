#include "tx.h"

bool tx::visited(page_id_t page_id) const {
    return visit;//before.find(page_id) != before.end();
}

void tx::first_log(page_id_t page_id, const page &val) {
    before[page_id] = log_obj(val);
    visit = true;
}

void tx::last_log(page_id_t page_id, const page &val) {
    after[page_id] = log_obj(val);
}

tx::tx(tx_id_t tx_id) : tx_id(tx_id) { }

bool tx::operator<(const tx &that) const {
    return tx_id < that.tx_id;
}

