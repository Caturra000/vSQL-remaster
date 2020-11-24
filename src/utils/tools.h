#ifndef VSQL_TOOLS_H
#define VSQL_TOOLS_H

#include <vector>
#include <iostream>
#include <ctime>
#include <random>

#define force_inline __inline__ __attribute__((always_inline))
#define likely(x) __builtin_expect(!!(x),1)
#define unlikely(x) __builtin_expect(!!(x),0)
#define check_if_false(condition) ((void)sizeof(char[1 - 2*!!(condition)]))
#define offset_of(type, member) （(size_t)(&(((type*)0)->member)))

#define MY_DEBUG_ON


#ifdef MY_DEBUG_ON
#define debug_only(a) do { std::cout << "[**DEBUG MODE**]" << std::endl; a } while(false)
#endif
#ifndef MY_DEBUG_ON
#define debug_only(a)
#endif


class string_utils {
public:
    // return a vector containing {lo,hi} pairs which are split by cut
    static std::vector<std::pair<int,int>>
    split(const std::string &str, const char cut) {
        auto vec = std::vector<std::pair<int,int>>();
        return split(str,cut,vec);
    }

    // 把左右全部截掉，默认是空格
    static std::string
    trim(const std::string &str, const char cut = ' ') {
        if(str.size() == 0) return str;
        auto lo = 0;
        auto hi = str.size() - 1;
        while(lo <= hi && str[lo] == cut) ++lo;
        while(hi >= lo && str[hi] == cut) --hi;
        return str.substr(lo, hi - lo + 1); // lo == size 也保证正确性
    }

    // 方便测试用
    // 由于可能用到文件路径方面，所以还是只用简单的字母
    static std::string random(int length) {
        static std::mt19937 mt(std::time(nullptr));
        std::string str;
        for(int i = 0; i < length; ++i) {
            char c = 'a' + mt()%26;
            str += c;
        }
        return str;
    }


private:
    // check if invalid and find the first valid index
    static std::vector<std::pair<int,int>>
    split(const std::string &str, const char cut,
          std::vector<std::pair<int,int>> &vec) {
        auto i = 0;
        while(i != str.size() && str[i] == cut) ++i;
        if(i == str.size()) return { };
        return split(i,str,cut,vec);
    }

    static std::vector<std::pair<int,int>>&
    split(int start, const std::string &str, const char cut,
          std::vector<std::pair<int,int>> &vec) {
        auto i = start;
        while(i != str.size() && str[i] != cut) ++i;
        vec.emplace_back(start,i);
        while(i != str.size() && str[i] == cut) ++i;
        if(i == str.size()) return vec;
        return split(i,str,cut,vec);
    }


};

class container_utils {
public:
};



#endif //VSQL_TOOLS_H
