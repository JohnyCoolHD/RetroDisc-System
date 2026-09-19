#include "runtime_internal.hpp"

#include <filesystem>
#include <iostream>
#include <string>
#include <system_error>


namespace runtime_internal
{


/*
    ================================================================
    MIGRATE AND REDIRECT LEGACY WINDOWS USER
    ================================================================

    RetroDisc always launches Wine/Proton with WINEUSERNAME/
    USERPROFILE/etc. pointed at the canonical "RetroDisc" profile.
    That is enough for plain Wine, but Proton does NOT reliably
    honor it: Proton hardcodes its own internal Windows user to
    "steamuser" for a range of its own internal bookkeeping and for
    some Win32 known-folder resolution used by many games (including
    Unity's Application.persistentDataPath, which is exactly what
    Hollow Knight uses for its save files under
    "AppData/LocalLow/Team Cherry/Hollow Knight"). Stock Wine can
    similarly fall back to the actual Unix account name in some
    circumstances instead of the requested WINEUSERNAME.

    Concretely, this means saves can end up physically written under
        drive_c/users/steamuser/...      (observed under Proton)
    or
        drive_c/users/<unix-username>/...  (observed under Wine)
    instead of the intended, persisted
        drive_c/users/RetroDisc/...

    Both "steamuser" and the actual Unix account name already exist
    as real directories in the immutable base prefix (created by the
    base's own "wineboot" in initializeGlobalPrefix()), so they are
    visible in every merged overlay regardless of which game/datapath
    is running. Any writes under them therefore still land in THIS
    game's persistent upper -- they are not lost -- but they end up
    at a different path than the canonical "RetroDisc" profile, so a
    later launch under a different runtime (which may resolve the
    legacy name differently) appears to have "lost" the save.

    The fix: make each known legacy name a symlink to "RetroDisc",
    migrating any real content already sitting there (e.g. from
    earlier launches before this fix existed, or from this exact
    kind of runtime switch) into the canonical profile first so
    nothing is silently discarded.

    IMPORTANT -- WRITE THROUGH THE MOUNTED OVERLAY, NOT THE RAW UPPER:

    All entries that this function CREATES OR REMOVES (the redirect
    symlink itself, and the migrated-into canonical profile) are
    written through "usersDirectory", i.e. through the already-
    mounted fuse-overlayfs merged view -- never directly on the raw
    persistent upper directory on disk.

    fuse-overlayfs keeps its own userspace state about the merged
    tree for the lifetime of the mount. Modifying the upper directory
    out-of-band (bypassing the mount) while it is mounted is not
    guaranteed to become visible through the merged view afterwards.
    Proton is launched against the MERGED path, so if the "steamuser"
    redirect were created out-of-band and the merged view still
    showed the old (pre-redirect) state, Proton's own internal
    "steamuser" migration would run against incomplete paths and
    crash with FileNotFoundError -- this was observed in practice.

    Reading the ORIGINAL template content still happens directly
    from "sourceUsersDirectory" (the immutable global lower prefix).
    That is read-only and therefore safe regardless of mount state.
*/

bool migrateAndSymlinkLegacyUser(
    const std::filesystem::path& usersDirectory,
    const std::filesystem::path& persistentUsersDirectory,
    const std::filesystem::path& sourceUsersDirectory,
    const std::string& legacyName
)
{
    if(
        legacyName.empty() ||
        legacyName == CANONICAL_WINDOWS_USER
    )
    {
        return true;
    }

    /*
        "usersDirectory" is the MOUNTED merged view. It is used both
        to determine what Wine/Proton currently exposes AND as the
        target for every write this function performs -- see the
        big comment above the function for why writes must go
        through the mount instead of the raw upper.
    */
    const auto legacyPath =
        usersDirectory /
        legacyName;

    const auto sourceLegacyPath =
        sourceUsersDirectory /
        legacyName;

    /*
        These two intentionally point through the MOUNTED merged
        view ("usersDirectory"), not through "persistentUsersDirectory"
        (the raw upper). See the note above: every write in this
        function must go through the active fuse-overlayfs mount so
        that its own view of the tree stays consistent, since Proton
        is about to be launched against that exact merged view.

        "persistentUsersDirectory" is kept as a parameter only for
        the stale-entry sanity checks a caller may still want to run
        against the raw upper after unmount; it is no longer used to
        write here.
    */
    const auto persistentLegacyPath =
        legacyPath;

    const auto persistentCanonicalPath =
        usersDirectory /
        CANONICAL_WINDOWS_USER;

    (void)persistentUsersDirectory;

    std::error_code ec;

    /*
        ============================================================
        INSPECT MERGED LEGACY PROFILE
        ============================================================
    */

    const auto status =
        std::filesystem::symlink_status(
            legacyPath,
            ec
        );

    if(
        status.type() ==
        std::filesystem::file_type::not_found
    )
    {
        ec.clear();
    }
    else if(ec)
    {
        std::cerr
            << "Could not inspect legacy Windows user directory:"
            << std::endl
            << "    "
            << legacyPath
            << std::endl
            << "    "
            << ec.message()
            << std::endl;

        return false;
    }

    /*
        ============================================================
        NOTHING IN MERGED PREFIX
        ============================================================
    */

    if(
        status.type() ==
        std::filesystem::file_type::not_found
    )
    {
        std::error_code upperEc;

        const auto upperStatus =
            std::filesystem::symlink_status(
                persistentLegacyPath,
                upperEc
            );

        if(
            upperEc &&
            upperEc !=
                std::make_error_code(
                    std::errc::no_such_file_or_directory
                )
        )
        {
            std::cerr
                << "Could not inspect persistent legacy"
                << " Windows user:"
                << std::endl
                << "    "
                << persistentLegacyPath
                << std::endl
                << "    "
                << upperEc.message()
                << std::endl;

            return false;
        }

        if(
            !upperEc &&
            upperStatus.type() !=
                std::filesystem::file_type::not_found
        )
        {
            upperEc.clear();

            std::filesystem::remove_all(
                persistentLegacyPath,
                upperEc
            );

            if(upperEc)
            {
                std::cerr
                    << "Could not remove stale persistent"
                    << " legacy Windows user:"
                    << std::endl
                    << "    "
                    << persistentLegacyPath
                    << std::endl
                    << "    "
                    << upperEc.message()
                    << std::endl;

                return false;
            }
        }

        upperEc.clear();

        std::filesystem::create_directory_symlink(
            CANONICAL_WINDOWS_USER,
            persistentLegacyPath,
            upperEc
        );

        if(upperEc)
        {
            std::cerr
                << "Could not create legacy Windows user redirect:"
                << std::endl
                << "    "
                << persistentLegacyPath
                << std::endl
                << "    "
                << upperEc.message()
                << std::endl;

            return false;
        }

        return true;
    }

    /*
        ============================================================
        ALREADY A SYMLINK
        ============================================================
    */

    if(std::filesystem::is_symlink(status))
    {
        std::error_code targetEc;

        const auto target =
            std::filesystem::read_symlink(
                legacyPath,
                targetEc
            );

        const bool alreadyCorrect =
            !targetEc &&
            (
                target ==
                    std::filesystem::path(
                        CANONICAL_WINDOWS_USER
                    ) ||
                target.filename() ==
                    CANONICAL_WINDOWS_USER
            );

        if(alreadyCorrect)
        {
            return true;
        }

        std::error_code upperEc;

        std::filesystem::remove_all(
            persistentLegacyPath,
            upperEc
        );

        if(upperEc)
        {
            std::cerr
                << "Could not remove stale persistent legacy"
                << " Windows user:"
                << std::endl
                << "    "
                << persistentLegacyPath
                << std::endl
                << "    "
                << upperEc.message()
                << std::endl;

            return false;
        }

        upperEc.clear();

        std::filesystem::create_directory_symlink(
            CANONICAL_WINDOWS_USER,
            persistentLegacyPath,
            upperEc
        );

        if(upperEc)
        {
            std::cerr
                << "Could not recreate legacy Windows user redirect:"
                << std::endl
                << "    "
                << persistentLegacyPath
                << std::endl
                << "    "
                << upperEc.message()
                << std::endl;

            return false;
        }

        return true;
    }

    /*
        ============================================================
        REAL DIRECTORY
        ============================================================

        IMPORTANT:

        Never copy from legacyPath here.

        legacyPath points into the FUSE merged prefix:

            /tmp/.../merged_prefix/...

        The actual source is sourceLegacyPath, which points directly
        into the immutable global lower prefix (read-only, safe).

        The destination is the MOUNTED merged view (persistentCanonicalPath,
        which now resolves through usersDirectory), so the write goes
        through fuse-overlayfs and its view of the upper stays consistent.
    */

    if(std::filesystem::is_directory(status))
    {
        std::error_code sourceEc;

        const auto sourceStatus =
            std::filesystem::symlink_status(
                sourceLegacyPath,
                sourceEc
            );

        if(
            sourceStatus.type() ==
            std::filesystem::file_type::not_found
        )
        {
            /*
                The profile may already exist only in the game upper.
                In that case there is nothing to migrate from the
                immutable base.
            */
            sourceEc.clear();
        }
        else if(sourceEc)
        {
            std::cerr
                << "Could not inspect source Windows profile:"
                << std::endl
                << "    "
                << sourceLegacyPath
                << std::endl
                << "    "
                << sourceEc.message()
                << std::endl;

            return false;
        }

        if(
            !sourceEc &&
            std::filesystem::is_directory(sourceStatus)
        )
        {
            std::cout
                << "Migrating legacy Windows profile into canonical"
                << " RetroDisc profile:"
                << std::endl
                << "    "
                << sourceLegacyPath
                << std::endl
                << " -> "
                << persistentCanonicalPath
                << std::endl;

            std::error_code copyEc;

            std::filesystem::create_directories(
                persistentCanonicalPath,
                copyEc
            );

            if(copyEc)
            {
                std::cerr
                    << "Could not create persistent RetroDisc profile:"
                    << std::endl
                    << "    "
                    << persistentCanonicalPath
                    << std::endl
                    << "    "
                    << copyEc.message()
                    << std::endl;

                return false;
            }

            copyEc.clear();

            std::filesystem::copy(
                sourceLegacyPath,
                persistentCanonicalPath,
                std::filesystem::copy_options::recursive |
                std::filesystem::copy_options::skip_existing,
                copyEc
            );

            if(copyEc)
            {
                std::cerr
                    << "Could not migrate legacy Windows profile:"
                    << std::endl
                    << "    "
                    << sourceLegacyPath
                    << std::endl
                    << "    "
                    << copyEc.message()
                    << std::endl;

                return false;
            }
        }

        /*
            Replace only the persistent upper entry.

            The lower directory is never removed.
        */

        std::error_code upperEc;

        const auto upperStatus =
            std::filesystem::symlink_status(
                persistentLegacyPath,
                upperEc
            );

        if(
            upperEc &&
            upperEc !=
                std::make_error_code(
                    std::errc::no_such_file_or_directory
                )
        )
        {
            std::cerr
                << "Could not inspect persistent legacy"
                << " Windows user:"
                << std::endl
                << "    "
                << persistentLegacyPath
                << std::endl
                << "    "
                << upperEc.message()
                << std::endl;

            return false;
        }

        if(
            !upperEc &&
            upperStatus.type() !=
                std::filesystem::file_type::not_found
        )
        {
            upperEc.clear();

            std::filesystem::remove_all(
                persistentLegacyPath,
                upperEc
            );

            if(upperEc)
            {
                std::cerr
                    << "Could not remove persistent legacy"
                    << " Windows user:"
                    << std::endl
                    << "    "
                    << persistentLegacyPath
                    << std::endl
                    << "    "
                    << upperEc.message()
                    << std::endl;

                return false;
            }
        }

        upperEc.clear();

        std::filesystem::create_directory_symlink(
            CANONICAL_WINDOWS_USER,
            persistentLegacyPath,
            upperEc
        );

        if(upperEc)
        {
            std::cerr
                << "Could not create legacy Windows user redirect"
                << " after migration:"
                << std::endl
                << "    "
                << persistentLegacyPath
                << std::endl
                << "    "
                << upperEc.message()
                << std::endl;

            return false;
        }

        return true;
    }

    /*
        ============================================================
        UNEXPECTED ENTRY
        ============================================================
    */

    std::cerr
        << "Legacy Windows user path is neither a directory nor"
        << " a symlink, leaving it untouched:"
        << std::endl
        << "    "
        << legacyPath
        << std::endl;

    return true;
}


} // namespace runtime_internal
