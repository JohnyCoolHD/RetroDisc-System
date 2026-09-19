#include "runtime_internal.hpp"

#include <filesystem>
#include <iostream>
#include <vector>


namespace runtime_internal
{


bool prepareCanonicalUserDirectory(
    const std::filesystem::path& usersDirectory
)
{
    const auto canonicalUser =
        usersDirectory /
        CANONICAL_WINDOWS_USER;

    /*
        Ensure:

            drive_c/users
    */

    if(!ensureDirectory(usersDirectory))
    {
        std::cerr
            << "Could not prepare Wine users directory:"
            << std::endl
            << "    "
            << usersDirectory
            << std::endl;

        return false;
    }

    /*
        Ensure:

            drive_c/users/RetroDisc

        This is where the old implementation failed when RetroDisc
        contained broken symlinks.
    */

    if(!ensureDirectory(canonicalUser))
    {
        std::cerr
            << "Could not prepare canonical RetroDisc directory:"
            << std::endl
            << "    "
            << canonicalUser
            << std::endl;

        return false;
    }

    /*
        ============================================================
        CANONICAL USER DIRECTORIES
        ============================================================
    */

    const std::vector<std::filesystem::path> directories =
    {
        canonicalUser / "Documents",
        canonicalUser / "AppData",
        canonicalUser / "AppData" / "Roaming",
        canonicalUser / "AppData" / "Local",
        canonicalUser / "Desktop",
        canonicalUser / "Downloads",
        canonicalUser / "Pictures",
        canonicalUser / "Music",
        canonicalUser / "Videos"
    };

    for(const auto& directory : directories)
    {
        if(!ensureDirectory(directory))
        {
            std::cerr
                << "Could not prepare canonical directory:"
                << std::endl
                << "    "
                << directory
                << std::endl;

            return false;
        }
    }

    /*
        ============================================================
        LEGACY (WINDOWS-XP-STYLE) COMPAT REDIRECTS
        ============================================================

        Required so Proton's own "steamuser" migration never has to
        create these itself. See ensureLegacyCompatRedirect() above.
    */

    if(!ensureLegacyCompatRedirect(
        canonicalUser,
        std::filesystem::path("Local Settings") / "Application Data",
        "../AppData/Local"
    ))
    {
        return false;
    }

    if(!ensureLegacyCompatRedirect(
        canonicalUser,
        "Application Data",
        "AppData/Roaming"
    ))
    {
        return false;
    }

    if(!ensureLegacyCompatRedirect(
        canonicalUser,
        "My Documents",
        "Documents"
    ))
    {
        return false;
    }

    return true;
}


} // namespace runtime_internal
