#include "prefix_internal.hpp"

#include <cstdlib>
#include <filesystem>


namespace prefix_internal
{


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


    return std::filesystem::path(
        home
    );
}


} // namespace prefix_internal
