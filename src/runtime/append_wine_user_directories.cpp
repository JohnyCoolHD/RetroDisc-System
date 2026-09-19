#include "runtime_internal.hpp"
#include "context.hpp"

#include <filesystem>
#include <sstream>
#include <vector>


namespace runtime_internal
{


/*
    ================================================================
    WINE USER DIRECTORIES
    ================================================================
*/

bool appendWineUserDirectories(
    std::ostringstream& command,
    const Context& ctx
)
{
    const auto userDirectory =
        ctx.prefixMergedPfxDirectory /
        "drive_c" /
        "users" /
        CANONICAL_WINDOWS_USER;

    const std::vector<std::filesystem::path> directories =
    {
        userDirectory / "Documents",
        userDirectory / "AppData",
        userDirectory / "AppData" / "Roaming",
        userDirectory / "AppData" / "Local",
        userDirectory / "Desktop",
        userDirectory / "Downloads",
        userDirectory / "Pictures",
        userDirectory / "Music",
        userDirectory / "Videos"
    };

    /*
        Do not use plain mkdir -p blindly.

        The C++ preparation above has already repaired broken
        canonical symlinks.

        mkdir -p is safe here and also protects against a directory
        disappearing between preparation and launch.
    */

    command
        << "mkdir -p";

    for(const auto& directory : directories)
    {
        command
            << " "
            << shellQuote(
                directory.string()
            );
    }

    command
        << " && ";

    return true;
}


} // namespace runtime_internal
