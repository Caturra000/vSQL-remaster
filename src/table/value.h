#ifndef VSQL_VALUE_H
#define VSQL_VALUE_H

#include <cstdint>

#define value_assign(member, type)  \
value& operator = (type _val) {     \
    member = _val;                  \
    return *this;                   \
}

// pod
class value {
public:
    union {
//        int8_t  i8;
//        int16_t i16;
        int32_t i32;
        int64_t i64;
        float   f32;
        double  f64;
        char*   ptr;
    };

    value_assign(i32, int);
    value_assign(i64, long long int);
    value_assign(f32, float);
    value_assign(f64, double);


};

#undef value_assign

#endif //VSQL_VALUE_H
