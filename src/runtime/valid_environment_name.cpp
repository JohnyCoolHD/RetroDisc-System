#include "runtime_internal.hpp"

#include <cstddef>
#include <string>


namespace runtime_internal
{


/*
    ================================================================
    ENVIRONMENT VALIDATION
    ================================================================
*/

bool validEnvironmentName(
    const std::string& name
)
{
    if(name.empty())
    {
        return false;
    }

    const char first =
        name[0];

    if(
        !(
            first == '_' ||
            (first >= 'A' && first <= 'Z') ||
            (first >= 'a' && first <= 'z')
        )
    )
    {
        return false;
    }

    for(std::size_t i = 1; i < name.size(); ++i)
    {
        const char c =
            name[i];

        if(
            !(
                c == '_' ||
                (c >= 'A' && c <= 'Z') ||
                (c >= 'a' && c <= 'z') ||
                (c >= '0' && c <= '9')
            )
        )
        {
            return false;
        }
    }

    return true;
}


} // namespace runtime_internal
