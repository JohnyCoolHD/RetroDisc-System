#include "prefix_internal.hpp"

#include <filesystem>
#include <string>


namespace prefix_internal
{


/*
    ================================================================
    RESOLVE GLOBAL PREFIX DIRECTORY
    ================================================================
*/


std::filesystem::path resolveGlobalPrefixDirectory(
    const std::filesystem::path& home,
    const std::string& runtimeFolder,
    const std::string& versionIdentifier
)
{
    return
        home /
        ".RetroDisc" /
        "prefix" /
        runtimeFolder /
        versionIdentifier;
}


} // namespace prefix_internal
