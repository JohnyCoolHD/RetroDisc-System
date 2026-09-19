#include "runtime_internal.hpp"
#include "context.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <system_error>


namespace runtime_internal
{


/*
    ================================================================
    PREPARE RUNTIME USER
    ================================================================
*/

bool prepareRuntimeUser(
    const Context& ctx,
    const std::string& runtimeUser
)
{
    if(
        ctx.prefixOverlayDirectory.empty() ||
        runtimeUser != CANONICAL_WINDOWS_USER
    )
    {
        return false;
    }

    const auto usersDirectory =
        ctx.prefixMergedPfxDirectory /
        "drive_c" /
        "users";

    const auto sourceUsersDirectory =
        ctx.prefixLowerDirectory /
        (
            ctx.runtime == "proton"
                ? std::filesystem::path("pfx")
                : std::filesystem::path()
        ) /
        "drive_c" /
        "users";

    const auto persistentUser =
        usersDirectory /
        CANONICAL_WINDOWS_USER;

    /*
        ============================================================
        CANONICAL USER
        ============================================================
    */

    if(!prepareCanonicalUserDirectory(
        usersDirectory
    ))
    {
        return false;
    }

    /*
        ============================================================
        VERIFY
        ============================================================
    */

    std::error_code ec;

    if(
        !std::filesystem::is_directory(
            persistentUser,
            ec
        ) ||
        ec
    )
    {
        std::cerr
            << "Persistent RetroDisc user does not exist after"
            << " preparation:"
            << std::endl
            << "    "
            << persistentUser
            << std::endl;

        return false;
    }

    const auto persistentUsersDirectory =
        ctx.prefixOverlayDirectory /
        "pfx" /
        "drive_c" /
        "users";

    /*
        ============================================================
        LEGACY USER REDIRECTS
        ============================================================

        Proton hardcodes "steamuser" for parts of its own internal
        user handling, and stock Wine can fall back to the actual
        Unix account name instead of honoring WINEUSERNAME in some
        circumstances. Both names are made symlinks into the
        canonical RetroDisc profile (migrating any real content
        already sitting there first) so that whichever one Wine or
        Proton actually ends up using internally, the data still
        lands in -- and is read back from -- the one persistent
        profile, regardless of which runtime is active.

        RetroDisc itself is always the canonical, persistent Windows
        user; nothing here creates a second independent profile.
    */

    if(!migrateAndSymlinkLegacyUser(
        usersDirectory,
        persistentUsersDirectory,
        sourceUsersDirectory,
        "steamuser"
    ))
    {
        return false;
    }

    const char* unixUserEnv =
        std::getenv("USER");

    if(!unixUserEnv || *unixUserEnv == '\0')
    {
        unixUserEnv =
            std::getenv("LOGNAME");
    }

    if(unixUserEnv && *unixUserEnv != '\0')
    {
        if(!migrateAndSymlinkLegacyUser(
            usersDirectory,
            persistentUsersDirectory,
            sourceUsersDirectory,
            unixUserEnv
        ))
        {
            return false;
        }
    }

    std::cout
        << "Runtime user:"
        << std::endl
        << "    "
        << CANONICAL_WINDOWS_USER
        << std::endl;

    std::cout
        << "Persistent user:"
        << std::endl
        << "    "
        << CANONICAL_WINDOWS_USER
        << std::endl;

    std::cout
        << "Canonical Windows profile:"
        << std::endl
        << "    "
        << persistentUser
        << std::endl;

    return true;
}


} // namespace runtime_internal
