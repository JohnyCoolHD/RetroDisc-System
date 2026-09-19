#include "runtime_internal.hpp"

#include <filesystem>
#include <iostream>
#include <system_error>


namespace runtime_internal
{


/*
    ================================================================
    PREPARE CANONICAL USER DIRECTORY
    ================================================================
*/

/*
    ================================================================
    ENSURE LEGACY COMPAT REDIRECT
    ================================================================

    Proton hardcodes its own migration of Windows-XP-style shell
    folder paths for the "steamuser" account (see Proton's
    migrate_user_paths()):

        Local Settings/Application Data -> ../AppData/Local
        Application Data                -> ./AppData/Roaming
        My Documents                    -> ./Documents

    Proton's own implementation of this is not robust against the
    parent directory being missing: if e.g. "Local Settings" does
    not already exist, Proton's internal makedirs() call fails
    (Proton silently swallows that OSError) and the following
    os.symlink() call then crashes with FileNotFoundError, aborting
    the whole launch.

    Since RetroDisc always redirects "steamuser" onto the canonical
    "RetroDisc" profile, and that profile is a fresh one that no
    wineboot has ever XP-initialized, these legacy paths never
    existed on it. RetroDisc therefore creates them itself so that
    Proton's own migration finds everything already in place and
    becomes a safe no-op instead of a crash source.
*/

bool ensureLegacyCompatRedirect(
    const std::filesystem::path& canonicalUser,
    const std::filesystem::path& relativeLegacyPath,
    const std::filesystem::path& linkTarget
)
{
    const auto legacyPath =
        canonicalUser /
        relativeLegacyPath;

    if(!ensureDirectory(legacyPath.parent_path()))
    {
        std::cerr
            << "Could not prepare legacy compat parent directory:"
            << std::endl
            << "    "
            << legacyPath.parent_path()
            << std::endl;

        return false;
    }

    std::error_code ec;

    const auto status =
        std::filesystem::symlink_status(
            legacyPath,
            ec
        );

    if(
        !ec &&
        status.type() !=
            std::filesystem::file_type::not_found
    )
    {
        /*
            Something is already there (a correct redirect from an
            earlier launch, or pre-existing real data). Never
            overwrite existing entries here; only fill the gap when
            nothing exists yet.
        */

        return true;
    }

    ec.clear();

    std::filesystem::create_directory_symlink(
        linkTarget,
        legacyPath,
        ec
    );

    if(
        ec &&
        ec !=
            std::make_error_code(
                std::errc::file_exists
            )
    )
    {
        std::cerr
            << "Could not create legacy compat redirect:"
            << std::endl
            << "    "
            << legacyPath
            << std::endl
            << " -> "
            << linkTarget
            << std::endl
            << "    "
            << ec.message()
            << std::endl;

        return false;
    }

    return true;
}


} // namespace runtime_internal
