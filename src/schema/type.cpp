//#include "type.h"
//
//
//const std::string &col_type::type_name() {
//    return TYPE_NAMES[magic];
//}
//
//
//std::size_t col_type::size() {
//    return TYPE_SIZES[magic];
//}
//
//col_type::col_type(VALUE_TYPE type)
//        : magic(type) { }
//
//col_type col_type::get_type(VALUE_TYPE value_type) {
//    static col_type init[] = {
//            [INT]        =   INT,
//            [CHAR]       =   CHAR,
//            [VARCHAR]    =   VARCHAR,
//            [BIGINT]     =   BIGINT,
//            [BOOLEAN]    =   BOOLEAN,
//            [DECIMAL]    =   DECIMAL,
//            [SMALLINT]   =   SMALLINT,
//            [TINYINT]    =   TINYINT,
//            [UNKNOWN]    =   UNKNOWN
//    };
//    return init[value_type];
//}
//
//col_type col_type::get_type_by_string(const std::string &str) {
////    static trie dict('Z'-'A'+1,'A', TYPE_NAMES);
//    if(str.empty()) return UNKNOWN;
//    if(str[0] == 'I') return str == TYPE_NAMES[INT] ? INT : UNKNOWN;
//    if(str[0] == 'C') return str == TYPE_NAMES[CHAR] ? CHAR : UNKNOWN;
//    if(str[0] == 'V') return str == TYPE_NAMES[VARCHAR] ? VARCHAR : UNKNOWN;
//    if(str[0] == 'D') return str == TYPE_NAMES[DECIMAL] ? DECIMAL : UNKNOWN;
//    if(str[0] == 'S') return str == TYPE_NAMES[SMALLINT] ? SMALLINT : UNKNOWN;
//    if(str[0] == 'T') return str == TYPE_NAMES[TINYINT] ? TINYINT : UNKNOWN;
//    if(str[0] == 'B') {
//        if(str == TYPE_NAMES[BIGINT]) return BIGINT;
//        if(str == TYPE_NAMES[BOOLEAN]) return BOOLEAN;
//        return UNKNOWN;
//    }
//    return UNKNOWN;
//}
//
//value col_type::pack(sqlite3_value *v) {
//    // return VALUE_PACK[magic](v); // ok // but how to unpack with different return type
//    return value();
//}
//
