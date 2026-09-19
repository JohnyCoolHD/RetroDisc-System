#include "prefix_internal.hpp"

#include <filesystem>
#include <string>


namespace prefix_internal
{


/*
    ================================================================
    RESOLVE PROTON VERSION IDENTIFIER
    ================================================================
*/


std::string resolveProtonVersionIdentifier(
    const std::filesystem::path& protonBinary
)
{
    if(protonBinary.empty())
    {
        return "default";
    }


    const auto directoryName =
        protonBinary
            .parent_path()
            .filename()
            .string();


    return sanitizeIdentifier(
        directoryName
    );
}


} // namespace prefix_internal
