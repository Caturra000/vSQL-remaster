#ifndef VSQL_LOG_OBJ_H
#define VSQL_LOG_OBJ_H

#include "../page/page.h"

// 不完整的日志
class log_obj {
    //page_id_t page_id;
    char *backup;
public:
    log_obj();
    explicit log_obj(const page &raw);
};

#endif //VSQL_LOG_OBJ_H
