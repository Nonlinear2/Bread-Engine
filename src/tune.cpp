#include "tune.hpp"

namespace SPSA {

    bool is_number(const std::string& s)
{
    std::string::const_iterator it = s.begin();
    while (it != s.end() && std::isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
}

void try_set_value(std::string name, std::string value){

}





} // namespace SPSA


#ifdef IS_TUNE
    #define TUNEABLE(type, name, value, ...) type name = value;
#else
    #define TUNEABLE(type, name, value, ...) constexpr type name = value; 
#endif