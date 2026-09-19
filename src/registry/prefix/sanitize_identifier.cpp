#include "prefix_internal.hpp"

#include <string>


namespace prefix_internal
{


/*
    ================================================================
    SANITIZE IDENTIFIER
    ================================================================
*/


std::string sanitizeIdentifier(
    const std::string& raw
)
{
    std::string result;
    result.reserve(
        raw.size()
    );


    for(const char c : raw)
    {
        if(
            c == '/' ||
            c == '\\' ||
            c == '\0'
        )
        {
            result += '_';
        }
        else
        {
            result += c;
        }
    }


    const auto start =
        result.find_first_not_of(
            " \t\r\n"
        );

    const auto end =
        result.find_last_not_of(
            " \t\r\n"
        );


    if(start == std::string::npos)
    {
        return "default";
    }


    result =
        result.substr(
            start,
            end - start + 1
        );


    if(
        result.empty() ||
        result == "." ||
        result == ".."
    )
    {
        return "default";
    }


    return result;
}


} // namespace prefix_internal
