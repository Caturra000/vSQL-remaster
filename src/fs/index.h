#ifndef VSQL_INDEX_H
#define VSQL_INDEX_H



// 抽象的索引
// 目前是用于解决bplus_tree的template无法运行时推断的问题
// 也提供了未来可能加入的hash索引的公共接口支持（别想了不可能的）
//class index {
//
//    void add(const record& rec);
//
//    void remove();
//
//    void find();
//
//};

#endif //VSQL_INDEX_H
