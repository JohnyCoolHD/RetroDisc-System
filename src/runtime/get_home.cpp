#include "runtime_internal.hpp"

#include <cstdlib>
#include <filesystem>


namespace runtime_internal
{


/*
    ================================================================
    HOME
    ================================================================
*/

std::filesystem::path getHome()
{
    const char* home =
        std::getenv("HOME");

    if(
        home == nullptr ||
        *home == '\0'
    )
    {
        return {};
    }

    return std::filesystem::path(home);
}


} // namespace runtime_internal
