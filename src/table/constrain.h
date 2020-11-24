#ifndef VSQL_CONSTRAIN_H
#define VSQL_CONSTRAIN_H

#include <limits>
#include <iostream>

// T为数值类型
template<typename T>
class constrain {
    using index_type = T;
private:
    T lo, hi;
    bool lo_oi{}, hi_oi{};  // index_open interval
public:
    // 一个不可思议的坑点： 唯独浮点的min是返回最小的正数，要用lowest才能保证最小值（设计者脑子怎么想的？）

    enum cmp { EQ = 2, GT = 4, LE = 8, LT = 16, GE = 32, }; // less than / less equal ....

    constrain(uint32_t c, T v);
    constrain(T lo, T hi, bool lo_oi, bool hi_oi);
    constrain();

    constrain operator & (const constrain &that);
    constrain& operator = (const constrain &that) = default;

    void out();
    bool runnable() const;
    T get_lo() const;
    T get_hi() const;
    bool lo_open() const;
    bool hi_open() const;
    bool valid(T v) const;

    static const constrain& always_false();

//    static bool valid(const constrain &op, T v);



};

template<typename T>
constrain<T>::constrain(T lo, T hi, bool lo_oi, bool hi_oi)
        : lo(lo), hi(hi), lo_oi(lo_oi), hi_oi(hi_oi) { }

template<typename T>
constrain<T>::constrain(uint32_t c, T v) : constrain() {
    if(c == EQ) lo = hi = v;
    else if(c == LE) hi = v;
    else if(c == LT) hi = v, hi_oi = true;
    else if(c == GE) lo = v;
    else if(c == GT) lo = v, lo_oi = true;
}

template<typename T>
constrain<T>::constrain() : constrain(std::numeric_limits<T>::lowest(),
                                      std::numeric_limits<T>::max(), false, false) { }

template<typename T>
constrain<T> constrain<T>::operator&(const constrain &that) {
    auto _lo = std::max(this->lo, that.lo);
    auto _hi = std::min(this->hi, that.hi);

    bool _lo_oi, _hi_oi;

    if(this->lo == that.lo) _lo_oi = this->lo_oi || that.lo_oi;
    else _lo_oi = (this->lo < that.lo) ? that.lo_oi : this->lo_oi;

    if(this->hi == that.hi) _hi_oi = this->hi_oi || that.hi_oi;
    else _hi_oi = (this->hi < that.hi) ? this->hi_oi : that.hi_oi;

    return constrain(_lo, _hi, _lo_oi, _hi_oi);
}

template<typename T>
void constrain<T>::out() {
    std::cout << (lo_oi ? '(' : '[')
              << lo << ',' << hi
              << (hi_oi ? ')' : ']');
}

template<typename T>
const constrain<T> &constrain<T>::always_false() {
    static constrain af(std::numeric_limits<T>::max(),
                        std::numeric_limits<T>::lowest(), false, false);
    return af;
}

//template<typename T>
//bool constrain<T>::valid(const constrain &op, T v) {
//    if(v > op.lo && v < op.hi) return true;
//    if(v < op.lo || v > op.hi) return false;
//    if(v == op.lo && !op.lo_oi) return true;
//    return v == op.hi && !op.hi_oi;
//}

template<typename T>
bool constrain<T>::runnable() const {
    if(lo > hi) return false;
    return !(lo == hi && (lo_oi || hi_oi));
}

template<typename T>
T constrain<T>::get_lo() const {
    return lo;
}

template<typename T>
T constrain<T>::get_hi() const {
    return hi;
}

template<typename T>
bool constrain<T>::lo_open() const {
    return lo_oi;
}

template<typename T>
bool constrain<T>::hi_open() const {
    return hi_oi;
}

template<typename T>
bool constrain<T>::valid(T v) const {
    if(v > lo && v < hi) return true;
    if(v < lo || v > hi) return false;
    if(v == lo && !lo_oi) return true;
    return v == hi && !hi_oi;
}

#endif //VSQL_CONSTRAIN_H
