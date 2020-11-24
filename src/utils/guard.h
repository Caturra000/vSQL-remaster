#ifndef VSQL_GUARD_H
#define VSQL_GUARD_H

// 可用于简单的资源管理如内存泄漏控制，离开作用域时自动调用lambda
// auto g = guard::make([&](){});
class guard {
    template <typename F>
    class explicit_guard {
        F call_later;
    public:
        explicit explicit_guard(const F &call_later) : call_later(call_later) { } // 感觉完美转发一下比较好？
        ~explicit_guard() { call_later(); }
    };
public:
    template <typename F>
    static explicit_guard<F> make(F call_later) {
        return explicit_guard<F>(call_later); // move
    }
};


#endif //VSQL_GUARD_H
