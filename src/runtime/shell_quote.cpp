#include "runtime_internal.hpp"

#include <string>


namespace runtime_internal
{


/*
    ================================================================
    SHELL QUOTING
    ================================================================
*/

std::string shellQuote(
    const std::string& value
)
{
    std::string result = "'";

    for(const char c : value)
    {
        if(c == '\'')
        {
            result += "'\\''";
        }
        else
        {
            result += c;
        }
    }

    result += "'";

    return result;
}


} // namespace runtime_internal
