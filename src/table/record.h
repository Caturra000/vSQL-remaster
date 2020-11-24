#ifndef VSQL_RECORD_H
#define VSQL_RECORD_H

#include <vector>
#include <tuple>
#include "../defs/defs.h"
#include "value.h"

// 只负责内存暂存，不负责解析
// 解析由 parser 负责
class record {
public:
    using var_ptr = std::tuple<column_id_t, offset_t, size_t>; // i-th, offset, len
    // 记录第ith的列是因为当record解析时能利用这个信息而无需schema/col_type就能得知哪里的ptr内存需要回收
    // 虽然这个方法有点拐弯，但这要求values的ptr复制是浅拷贝而非深拷贝，因为value总是被容器带走

    row_id_t get_rowid() const;
    value get(int pos) const;
    const var_ptr& get_var(int pos) const;
    size_t get_raw_size() const;
    //record(const std::vector<value> &values, const std::vector<var_ptr> &var_info, size_t raw_size); // 这个接口有点偷懒，但避免了var每次strlen
    record(std::vector<value> values, std::vector<var_ptr> var_info, size_t raw_size, bool temp = false);
    ~record();
    static var_ptr make_var_ptr(column_id_t cid, offset_t off, size_t len);
private:
    row_id_t                   row_id; // 只有没索引时才会启用，反正会和values[0]重合
    std::vector<value>         values;
    std::vector<var_ptr>       var_info;
    size_t                     raw_size; // 作为raw_record时至少要分配的字节大小 // TODO 尚未添加对应构造
    bool                       temp; // 非临时状态会回收ptr资源，临时用于转移状态，不回收资源
};

#endif //VSQL_RECORD_H
