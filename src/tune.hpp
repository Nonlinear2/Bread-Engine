#pragma once

#include <string>
#include <unordered_map>
#include <set>
#include <stdexcept>

// #define IS_TUNE 1

namespace SPSA {

struct TuneableInfo {
    int* val_ptr;
    int min;
    int max;
};

// make sure values is initialized when the TUNEABLE macro runs
inline std::unordered_map<std::string, TuneableInfo>& get_values(){
    static std::unordered_map<std::string, TuneableInfo> values; // created once
    return values;
}

inline void register_tuneable(const std::string& name, int* val_ptr, int min, int max){
    auto& values = get_values();
    if (values.find(name) != values.end())
        throw std::runtime_error("duplicate tuneable: " + name);
    values[name] = {val_ptr, min, max};
}

inline bool is_registered(const std::string& name){
    auto& values = get_values();
    return values.find(name) != values.end();
}

inline void set_value(const std::string& name, int value){
    auto& values = get_values();
    if (values.find(name) == values.end())
        throw std::runtime_error("unknown tuneable: " + name);
    *values[name].val_ptr = value;
}

inline int get_value(const std::string& name){
    auto& values = get_values();
    auto it = values.find(name);
    if (it == values.end())
        throw std::runtime_error("tuneable not found: " + name);
    return *it->second.val_ptr;
}

} // namespace SPSA


#ifdef IS_TUNE
    #define TUNEABLE(name, type, value, min, max, c, r) \
        type name = value; \
        static bool name##_init = []() { \
            SPSA::register_tuneable(#name, &name, min, max); \
            std::cout << #name << ", " << #type << ", " << (float)(value) << ", " \
                      << (float)(min) << ", " << (float)(max) << ", " \
                      << (float)(c) << ", " << (float)(r) << std::endl; \
            return true; \
        }();
#else
    #define TUNEABLE(name, type, value, min, max, c, r) constexpr type name = value;
#endif

#define UNACTIVE_TUNEABLE(name, type, value, min, max, c, r) constexpr type name = value;