#include "discord_internal.hpp"

#include <cstdlib>
#include <filesystem>
#include <vector>


namespace discord_internal
{


/*
    ================================================================
    SOCKET DIRECTORIES
    ================================================================
*/

std::vector<std::filesystem::path>
getSocketDirectories()
{
    std::vector<std::filesystem::path>
        directories;

    const char* variables[] =
    {
        "XDG_RUNTIME_DIR",
        "TMPDIR",
        "TMP",
        "TEMP"
    };

    for(
        const char* variable :
        variables
    )
    {
        const char* value =
            std::getenv(variable);

        if(
            value != nullptr &&
            *value != '\0'
        )
        {
            directories.emplace_back(
                value
            );
        }
    }

    directories.emplace_back(
        "/tmp"
    );

    return directories;
}


} // namespace discord_internal
