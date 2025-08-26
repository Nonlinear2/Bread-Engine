#pragma once

#include <string>
#include <unordered_map>
#include <set>
#include <stdexcept>

// #define IS_TUNE 1

namespace SPSA {

// make sure values is initialized when the TUNEABLE macro runs
inline std::unordered_map<std::string, int*>& get_values(){
    static std::unordered_map<std::string, int*> values; // created once
    return values;
}

inline void register_tuneable(const std::string& name, int* ptr){
    auto& values = get_values();
    if (values.find(name) != values.end())
        throw std::runtime_error("duplicate tuneable: " + name);
    values[name] = ptr;
}

inline bool is_registered(const std::string& name){
    auto& values = get_values();
    return values.find(name) != values.end();
}

inline void set_value(const std::string& name, int value){
    auto& values = get_values();
    if (values.find(name) == values.end())
        throw std::runtime_error("unknown tuneable: " + name);
    *values[name] = value;
}

inline int get_value(const std::string& name){
    auto& values = get_values();
    auto it = values.find(name);
    if (it == values.end())
        throw std::runtime_error("tuneable not found: " + name);
    return *it->second;
}

} // namespace SPSA


#ifdef IS_TUNE
    #define TUNEABLE(name, type, value, min, max, c, r) \
        type name = value; \
        static bool name##_init = []() { \
            SPSA::register_tuneable(#name, &name); \
            std::cout << #name << ", " << #type << ", " << (float)(value) << ", " \
                      << (float)(min) << ", " << (float)(max) << ", " \
                      << (float)(c) << ", " << (float)(r) << std::endl; \
            return true; \
        }();
#else
    #define TUNEABLE(name, type, value, min, max, c, r) \
        constexpr type name = value;
#endif

#define UNACTIVE_TUNEABLE(name, type, value, min, max, c, r) \
        constexpr type name = value;