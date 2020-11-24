#ifndef VSQL_TYPE_H
#define VSQL_TYPE_H

#include <string>
#include <unordered_map>


enum col_type {
    UNKNOWN,
    INT,
    CHAR,
    VARCHAR,
    BIGINT,
    BOOLEAN,
    DOUBLE,
    SMALLINT,
    TINYINT,
};

class col_type_handler {

public:
    static col_type parse(const std::string &type_str) {
        static std::unordered_map<std::string, col_type> map { // 其实还是挺慢的，尽量只在建表时使用
                {"int",      col_type::INT},
                {"char",     col_type::CHAR},
                {"varchar",  col_type::VARCHAR},
                {"bigint",   col_type::BIGINT},
                {"boolean",  col_type::BOOLEAN},
                {"double",  col_type::DOUBLE},
                {"smallint", col_type::SMALLINT},
                {"tinyint",  col_type ::TINYINT}
        };
        return map[type_str];
    }
    static size_t size(col_type type) {
        //  不支持C99特性，改用SWITCH-CASE
//        static size_t map[] = {
//                /*[col_type::UNKNOWN]   = */1,
//                /*[col_type::INT]       = */4,
//                /*[col_type::CHAR]      = */8, // 指针大小
//                /*[col_type::VARCHAR]   = */8,
//                /*[col_type::BIGINT]    = */8,
//                /*[col_type::BOOLEAN]   = */4,
//                /*[col_type::DECIMAL]   = */8,
//                /*[col_type::SMALLINT]   = */4,
//                /*[col_type::TINYINT]  = */4,
//        };
        switch(type) {
            case UNKNOWN:
                return 1;
            case INT:
            case SMALLINT:
            case TINYINT:
            case BOOLEAN:
                return 4;
            case CHAR:
            case VARCHAR:
            case BIGINT:
            case DOUBLE:
                return 8;
        }
        // return map[type];
    }

};


#endif //VSQL_TYPE_H
