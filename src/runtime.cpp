#include "runtime.hpp"
#include "filesystem.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <sys/wait.h>

namespace
{

/*
    ================================================================
    CONSTANTS
    ================================================================
*/

constexpr const char* CANONICAL_WINDOWS_USER = "RetroDisc";


/*
    ================================================================
    SHELL QUOTING
    ================================================================
*/

std::string shellQuote(
    const std::string& value
)
{
    std::string result = "'";

    for(const char c : value)
    {
        if(c == '\'')
        {
            result += "'\\''";
        }
        else
        {
            result += c;
        }
    }

    result += "'";

    return result;
}


/*
    ================================================================
    COMMAND EXECUTION
    ================================================================
*/

int runCommand(
    const std::string& command
)
{
    std::cout
        << std::endl
        << "Executing:"
        << std::endl
        << "    "
        << command
        << std::endl
        << std::endl;

    return std::system(
        command.c_str()
    );
}


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


/*
    ================================================================
    STEAM ROOT
    ================================================================
*/

std::filesystem::path findSteamRoot()
{
    const auto home =
        getHome();

    if(home.empty())
    {
        return {};
    }

    const std::vector<std::filesystem::path> roots =
    {
        home / ".local" / "share" / "Steam",
        home / ".steam" / "root",
        home / ".steam" / "steam"
    };

    for(const auto& root : roots)
    {
        std::error_code ec;

        if(
            std::filesystem::is_directory(
                root / "steamapps",
                ec
            )
        )
        {
            return root;
        }
    }

    return {};
}

/*
    ================================================================
    PROTON
    ================================================================
*/

std::filesystem::path findProton(
    const Context& ctx
)
{
    const auto home =
        getHome();

    if(home.empty())
    {
        return {};
    }


    /*
        ============================================================
        EXPLICIT PROTON PATH
        ============================================================

        Explicit configuration always has priority.

        Supported:

            "path":
                "/path/to/proton"

        or:

            "path":
                "/path/to/GE-Proton11-6-x86_64"

        In the second case RetroDisc expects:

            /path/to/GE-Proton11-6-x86_64/proton
    */

    if(!ctx.protonPath.empty())
    {
        const auto configured =
            std::filesystem::path(
                ctx.protonPath
            );

        std::error_code ec;

        if(
            std::filesystem::is_regular_file(
                configured,
                ec
            )
        )
        {
            return configured;
        }

        if(
            std::filesystem::is_directory(
                configured,
                ec
            )
        )
        {
            const auto proton =
                configured /
                "proton";

            if(
                std::filesystem::is_regular_file(
                    proton,
                    ec
                )
            )
            {
                return proton;
            }
        }

        std::cerr
            << "Configured Proton path is invalid:"
            << std::endl
            << "    "
            << configured
            << std::endl;
    }


    /*
        ============================================================
        PROTON VERSION
        ============================================================
    */

    if(ctx.protonVersion.empty())
    {
        return {};
    }


    /*
        ============================================================
        SEARCH ROOTS
        ============================================================

        Proton installations can come from:

            - Steam
            - ProtonUp-Qt
            - manual installations
            - other compatibility-tool managers

        RetroDisc therefore does not care where Proton came from.

        It searches known Steam compatibility-tool locations
        for the exact requested version.
    */

    const std::vector<std::filesystem::path> roots =
    {
        home / ".local" / "share" / "Steam",
        home / ".steam" / "root",
        home / ".steam" / "steam"
    };


    /*
        ============================================================
        COMPATIBILITYTOOLS.D
        ============================================================

        Examples:

            ~/.local/share/Steam/
                compatibilitytools.d/
                    GE-Proton11-6-x86_64/
                        proton

            ~/.steam/root/
                compatibilitytools.d/
                    GE-Proton11-6-x86_64/
                        proton

            ~/.steam/steam/
                compatibilitytools.d/
                    GE-Proton11-6-x86_64/
                        proton

        This is the important search path for ProtonUp-Qt and
        manually installed compatibility tools.
    */

    for(const auto& root : roots)
    {
        const auto proton =
            root /
            "compatibilitytools.d" /
            ctx.protonVersion /
            "proton";

        std::error_code ec;

        if(
            std::filesystem::is_regular_file(
                proton,
                ec
            )
        )
        {
            return proton;
        }
    }


    /*
        ============================================================
        STEAMAPPS/COMMON
        ============================================================

        Also support Proton installations that live directly
        inside Steam's normal compatibility-tool directory:

            ~/.local/share/Steam/
                steamapps/common/
                    Proton - Experimental/
                        proton
    */

    for(const auto& root : roots)
    {
        const auto proton =
            root /
            "steamapps" /
            "common" /
            ctx.protonVersion /
            "proton";

        std::error_code ec;

        if(
            std::filesystem::is_regular_file(
                proton,
                ec
            )
        )
        {
            return proton;
        }
    }


    /*
        ============================================================
        PROTON NOT FOUND
        ============================================================
    */

    std::cerr
        << "Proton version not found:"
        << std::endl
        << "    "
        << ctx.protonVersion
        << std::endl;

    std::cerr
        << "Searched:"
        << std::endl;

    for(const auto& root : roots)
    {
        std::cerr
            << "    "
            << (
                root /
                "compatibilitytools.d" /
                ctx.protonVersion /
                "proton"
            )
            << std::endl;
    }

    for(const auto& root : roots)
    {
        std::cerr
            << "    "
            << (
                root /
                "steamapps" /
                "common" /
                ctx.protonVersion /
                "proton"
            )
            << std::endl;
    }

    return {};
}

/*
    ================================================================
    ENVIRONMENT VALIDATION
    ================================================================
*/

bool validEnvironmentName(
    const std::string& name
)
{
    if(name.empty())
    {
        return false;
    }

    const char first =
        name[0];

    if(
        !(
            first == '_' ||
            (first >= 'A' && first <= 'Z') ||
            (first >= 'a' && first <= 'z')
        )
    )
    {
        return false;
    }

    for(std::size_t i = 1; i < name.size(); ++i)
    {
        const char c =
            name[i];

        if(
            !(
                c == '_' ||
                (c >= 'A' && c <= 'Z') ||
                (c >= 'a' && c <= 'z') ||
                (c >= '0' && c <= '9')
            )
        )
        {
            return false;
        }
    }

    return true;
}


/*
    ================================================================
    PRINT ENVIRONMENT
    ================================================================
*/

void printEnvironment(
    const Context& ctx
)
{
    for(const auto& [key, value] : ctx.environment)
    {
        std::cout
            << "Environment: "
            << key
            << "="
            << value
            << std::endl;
    }
}


/*
    ================================================================
    DLL OVERRIDES
    ================================================================
*/

std::string buildDllOverrides(
    const Context& ctx
)
{
    if(ctx.wine.dllOverrides.empty())
    {
        return {};
    }

    std::ostringstream value;

    bool first = true;

    for(const auto& [dll, overrideValue] :
        ctx.wine.dllOverrides)
    {
        if(dll.empty())
        {
            continue;
        }

        if(!first)
        {
            value << ";";
        }

        value
            << dll
            << "="
            << overrideValue;

        first = false;
    }

    return value.str();
}


/*
    ================================================================
    USER ENVIRONMENT
    ================================================================
*/

bool appendUserEnvironment(
    std::ostringstream& command,
    const Context& ctx
)
{
    for(const auto& [key, value] :
        ctx.environment)
    {
        /*
            These variables are owned by RetroDisc.

            In particular, external configuration must never
            replace the canonical Windows profile.
        */

        if(
            key == "WINEPREFIX" ||
            key == "STEAM_COMPAT_DATA_PATH" ||
            key == "STEAM_COMPAT_CLIENT_INSTALL_PATH" ||
            key == "STEAM_COMPAT_INSTALL_PATH" ||
            key == "SteamAppId" ||
            key == "SteamGameId" ||
            key == "WINEUSERNAME" ||
            key == "USERNAME" ||
            key == "USERPROFILE" ||
            key == "APPDATA" ||
            key == "LOCALAPPDATA" ||
            key == "WINEDLLOVERRIDES" ||
            key == "WINEESYNC" ||
            key == "WINEFSYNC" ||
            key == "WINE_NTSYNC" ||
            key == "XDG_CONFIG_HOME" ||
            key == "XDG_DATA_HOME" ||
            key == "XDG_CACHE_HOME"
        )
        {
            continue;
        }

        if(!validEnvironmentName(key))
        {
            std::cerr
                << "Invalid environment variable name:"
                << std::endl
                << "    "
                << key
                << std::endl;

            return false;
        }

        command
            << "export "
            << key
            << "="
            << shellQuote(value)
            << " && ";
    }

    return true;
}


/*
    ================================================================
    RUNTIME USER
    ================================================================
*/

std::string determineRuntimeUser(
    const Context&
)
{
    /*
        Linux account and Windows/Wine account are deliberately
        different concepts.

        Linux:
            maxim

        Windows/Wine/Proton:
            RetroDisc

        RetroDisc is the ONLY canonical Windows profile.
    */

    return CANONICAL_WINDOWS_USER;
}


/*
    ================================================================
    FILESYSTEM ENTRY HELPERS
    ================================================================
*/

/*
    symlink_status() is deliberately used here instead of
    is_directory().

    A broken symlink returns a valid symlink_status() entry, but
    is_directory() on the symlink itself returns false.

    This is exactly the situation that caused:

        Existing symlink does not resolve to a directory

    We repair such entries instead of aborting the launch.
*/

bool removeBrokenOrInvalidEntry(
    const std::filesystem::path& path
)
{
    std::error_code ec;

    const auto status =
        std::filesystem::symlink_status(
            path,
            ec
        );

    if(ec)
    {
        /*
            If the entry does not exist, there is nothing to remove.
        */

        if(ec == std::errc::no_such_file_or_directory)
        {
            return true;
        }

        std::cerr
            << "Could not inspect filesystem entry:"
            << std::endl
            << "    "
            << path
            << std::endl
            << "Error:"
            << std::endl
            << "    "
            << ec.message()
            << std::endl;

        return false;
    }

    if(
        std::filesystem::is_symlink(status)
    )
    {
        /*
            Check whether the symlink resolves to a directory.

            filesystem::is_directory(path) follows the symlink.
        */

        std::error_code targetEc;

        const bool targetIsDirectory =
            std::filesystem::is_directory(
                path,
                targetEc
            );

        if(
            !targetEc &&
            targetIsDirectory
        )
        {
            return true;
        }

        /*
            Broken symlink or symlink to something that is not a
            directory.

            Remove it so the canonical directory can be recreated.
        */

        std::cout
            << "Removing invalid canonical symlink:"
            << std::endl
            << "    "
            << path
            << std::endl;

        std::error_code removeEc;

        std::filesystem::remove(
            path,
            removeEc
        );

        if(removeEc)
        {
            std::cerr
                << "Could not remove invalid symlink:"
                << std::endl
                << "    "
                << path
                << std::endl
                << "Error:"
                << std::endl
                << "    "
                << removeEc.message()
                << std::endl;

            return false;
        }

        return true;
    }

    /*
        Existing regular directory is valid.
    */

    if(
        std::filesystem::is_directory(status)
    )
    {
        return true;
    }

    /*
        A regular file at a location where a directory is required
        cannot be used.
    */

    std::cerr
        << "Canonical path is not a directory:"
        << std::endl
        << "    "
        << path
        << std::endl;

    return false;
}


/*
    ================================================================
    ENSURE DIRECTORY
    ================================================================
*/

bool ensureDirectory(
    const std::filesystem::path& directory
)
{
    if(directory.empty())
    {
        return false;
    }

    /*
        First inspect the entry itself. This handles broken symlinks
        correctly.
    */

    if(!removeBrokenOrInvalidEntry(directory))
    {
        return false;
    }

    std::error_code ec;

    if(
        std::filesystem::is_directory(
            directory,
            ec
        )
    )
    {
        return true;
    }

    /*
        create_directories() is used rather than create_directory()
        because parent directories may also be missing.
    */

    std::filesystem::create_directories(
        directory,
        ec
    );

    if(ec)
    {
        /*
            Race-safe second check.

            Another process may have created the directory between
            our checks.
        */

        std::error_code verifyEc;

        if(
            std::filesystem::is_directory(
                directory,
                verifyEc
            )
        )
        {
            return true;
        }

        std::cerr
            << "Could not create directory:"
            << std::endl
            << "    "
            << directory
            << std::endl
            << "Error:"
            << std::endl
            << "    "
            << ec.message()
            << std::endl;

        return false;
    }

    return true;
}


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
        The merged users directory is used only to determine what
        Wine currently exposes.

        Migration itself is NEVER performed through the FUSE mount.
    */
    const auto legacyPath =
        usersDirectory /
        legacyName;

    const auto sourceLegacyPath =
        sourceUsersDirectory /
        legacyName;

    const auto persistentLegacyPath =
        persistentUsersDirectory /
        legacyName;

    const auto persistentCanonicalPath =
        persistentUsersDirectory /
        CANONICAL_WINDOWS_USER;

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
        into the immutable global lower prefix.

        The destination is the persistent game upper.
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


/*
    ================================================================
    CLEANUP RUNTIME USER
    ================================================================
*/

void cleanupRuntimeUser(
    const Context&,
    const std::string&
)
{
    /*
        There is no temporary runtime user.

        The persistent RetroDisc profile must NEVER be deleted.
    */
}


/*
    ================================================================
    WINE REGISTRY
    ================================================================
*/

void appendWineRegAdd(
    std::ostringstream& command,
    const std::string& key,
    const std::string& valueName,
    const std::string& type,
    const std::string& value
)
{
    command
        << "wine reg add "
        << shellQuote(key)
        << " /v "
        << shellQuote(valueName)
        << " /t "
        << shellQuote(type)
        << " /d "
        << shellQuote(value)
        << " /f && ";
}


void appendWineRegDelete(
    std::ostringstream& command,
    const std::string& key,
    const std::string& valueName
)
{
    command
        << "wine reg delete "
        << shellQuote(key)
        << " /v "
        << shellQuote(valueName)
        << " /f 2>/dev/null || true && ";
}


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


/*
    ================================================================
    WINE SHELL FOLDERS
    ================================================================
*/

void appendWineShellFolderRegistry(
    std::ostringstream& command
)
{
    const std::string user =
        "C:\\users\\RetroDisc";

    const std::string documents =
        user + "\\Documents";

    const std::string roaming =
        user + "\\AppData\\Roaming";

    const std::string local =
        user + "\\AppData\\Local";

    const std::string desktop =
        user + "\\Desktop";

    const std::string downloads =
        user + "\\Downloads";

    const std::string pictures =
        user + "\\Pictures";

    const std::string music =
        user + "\\Music";

    const std::string videos =
        user + "\\Videos";


    /*
        ============================================================
        SHELL FOLDERS
        ============================================================
    */

    appendWineRegAdd(
        command,
        "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
        "Personal",
        "REG_SZ",
        documents
    );

    appendWineRegAdd(
        command,
        "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
        "My Documents",
        "REG_SZ",
        documents
    );

    appendWineRegAdd(
        command,
        "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
        "AppData",
        "REG_SZ",
        roaming
    );

    appendWineRegAdd(
        command,
        "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
        "Local AppData",
        "REG_SZ",
        local
    );

    appendWineRegAdd(
        command,
        "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
        "Desktop",
        "REG_SZ",
        desktop
    );

    appendWineRegAdd(
        command,
        "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
        "{374DE290-123F-4565-9164-39C4925E467B}",
        "REG_SZ",
        downloads
    );

    appendWineRegAdd(
        command,
        "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
        "My Pictures",
        "REG_SZ",
        pictures
    );

    appendWineRegAdd(
        command,
        "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
        "My Music",
        "REG_SZ",
        music
    );

    appendWineRegAdd(
        command,
        "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
        "My Video",
        "REG_SZ",
        videos
    );


    /*
        ============================================================
        USER SHELL FOLDERS
        ============================================================
    */

    appendWineRegAdd(
        command,
        "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders",
        "Personal",
        "REG_EXPAND_SZ",
        documents
    );

    appendWineRegAdd(
        command,
        "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders",
        "My Documents",
        "REG_EXPAND_SZ",
        documents
    );

    appendWineRegAdd(
        command,
        "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders",
        "AppData",
        "REG_EXPAND_SZ",
        roaming
    );

    appendWineRegAdd(
        command,
        "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders",
        "Local AppData",
        "REG_EXPAND_SZ",
        local
    );

    appendWineRegAdd(
        command,
        "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders",
        "Desktop",
        "REG_EXPAND_SZ",
        desktop
    );

    appendWineRegAdd(
        command,
        "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders",
        "{374DE290-123F-4565-9164-39C4925E467B}",
        "REG_EXPAND_SZ",
        downloads
    );

    appendWineRegAdd(
        command,
        "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders",
        "My Pictures",
        "REG_EXPAND_SZ",
        pictures
    );

    appendWineRegAdd(
        command,
        "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders",
        "My Music",
        "REG_EXPAND_SZ",
        music
    );

    appendWineRegAdd(
        command,
        "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders",
        "My Video",
        "REG_EXPAND_SZ",
        videos
    );
}


/*
    ================================================================
    WINE CONFIGURATION
    ================================================================
*/

bool appendWineConfiguration(
    std::ostringstream& command,
    const Context& ctx
)
{
    const auto& wine =
        ctx.wine;


    /*
        ============================================================
        SYNCHRONIZATION
        ============================================================
    */

    command
        << "export WINEESYNC="
        << shellQuote(
            wine.sync.esync ? "1" : "0"
        )
        << " && ";

    command
        << "export WINEFSYNC="
        << shellQuote(
            wine.sync.fsync ? "1" : "0"
        )
        << " && ";

    command
        << "export WINE_NTSYNC="
        << shellQuote(
            wine.sync.ntsync ? "1" : "0"
        )
        << " && ";


    /*
        ============================================================
        VIRTUAL DESKTOP
        ============================================================
    */

    if(wine.display.virtualDesktop.enabled)
    {
        const int width =
            wine.display.virtualDesktop.width > 0
                ? wine.display.virtualDesktop.width
                : 640;

        const int height =
            wine.display.virtualDesktop.height > 0
                ? wine.display.virtualDesktop.height
                : 480;

        const std::string desktopSize =
            std::to_string(width) +
            "x" +
            std::to_string(height);

        appendWineRegAdd(
            command,
            "HKCU\\Software\\Wine\\Explorer",
            "Desktop",
            "REG_SZ",
            "Default"
        );

        appendWineRegAdd(
            command,
            "HKCU\\Software\\Wine\\Explorer\\Desktops",
            "Default",
            "REG_SZ",
            desktopSize
        );
    }
    else
    {
        appendWineRegDelete(
            command,
            "HKCU\\Software\\Wine\\Explorer",
            "Desktop"
        );

        appendWineRegDelete(
            command,
            "HKCU\\Software\\Wine\\Explorer\\Desktops",
            "Default"
        );
    }


    /*
        ============================================================
        X11
        ============================================================
    */

    appendWineRegAdd(
        command,
        "HKCU\\Software\\Wine\\X11 Driver",
        "Managed",
        "REG_SZ",
        wine.display.window.managed ? "Y" : "N"
    );

    appendWineRegAdd(
        command,
        "HKCU\\Software\\Wine\\X11 Driver",
        "Decorated",
        "REG_SZ",
        wine.display.window.decorations ? "Y" : "N"
    );

    appendWineRegAdd(
        command,
        "HKCU\\Software\\Wine\\X11 Driver",
        "GrabFullscreen",
        "REG_SZ",
        wine.display.window.mouseCapture ? "Y" : "N"
    );


    /*
        ============================================================
        DPI
        ============================================================
    */

    if(wine.display.dpi > 0)
    {
        appendWineRegAdd(
            command,
            "HKCU\\Control Panel\\Desktop",
            "LogPixels",
            "REG_DWORD",
            std::to_string(
                wine.display.dpi
            )
        );
    }


    /*
        ============================================================
        DIRECT3D
        ============================================================
    */

    if(
        !wine.graphics.renderer.empty() &&
        wine.graphics.renderer != "auto"
    )
    {
        appendWineRegAdd(
            command,
            "HKCU\\Software\\Wine\\Direct3D",
            "renderer",
            "REG_SZ",
            wine.graphics.renderer
        );
    }
    else
    {
        appendWineRegDelete(
            command,
            "HKCU\\Software\\Wine\\Direct3D",
            "renderer"
        );
    }

    if(wine.graphics.videoMemory > 0)
    {
        appendWineRegAdd(
            command,
            "HKCU\\Software\\Wine\\Direct3D",
            "VideoMemorySize",
            "REG_SZ",
            std::to_string(
                wine.graphics.videoMemory
            )
        );
    }
    else
    {
        appendWineRegDelete(
            command,
            "HKCU\\Software\\Wine\\Direct3D",
            "VideoMemorySize"
        );
    }

    appendWineRegAdd(
        command,
        "HKCU\\Software\\Wine\\Direct3D",
        "strict_draw_ordering",
        "REG_SZ",
        wine.graphics.strictDrawOrdering
            ? "enabled"
            : "disabled"
    );


    /*
        ============================================================
        WINDOWS VERSION
        ============================================================

        Do NOT run winecfg -v here.

        The shared prefix is owned by both Wine and Proton.
        Reconfiguring the Windows version on every launch can
        destabilize the shared prefix.
    */


    /*
        ============================================================
        DLL OVERRIDES
        ============================================================
    */

    const std::string dllOverrides =
        buildDllOverrides(ctx);

    if(!dllOverrides.empty())
    {
        command
            << "export WINEDLLOVERRIDES="
            << shellQuote(
                dllOverrides
            )
            << " && ";
    }
    else
    {
        command
            << "unset WINEDLLOVERRIDES 2>/dev/null"
            << " && ";
    }


    /*
        ============================================================
        USER DIRECTORIES
        ============================================================
    */

    if(!appendWineUserDirectories(
        command,
        ctx
    ))
    {
        return false;
    }

    appendWineShellFolderRegistry(
        command
    );

    return true;
}


/*
    ================================================================
    WINE GAME LAUNCH
    ================================================================
*/

void appendWineGameLaunch(
    std::ostringstream& command,
    const Context& ctx
)
{
    const auto executable =
        (
            ctx.mergedDirectory /
            ctx.executable
        ).string();

    if(ctx.wine.display.virtualDesktop.enabled)
    {
        const int width =
            ctx.wine.display.virtualDesktop.width > 0
                ? ctx.wine.display.virtualDesktop.width
                : 640;

        const int height =
            ctx.wine.display.virtualDesktop.height > 0
                ? ctx.wine.display.virtualDesktop.height
                : 480;

        const std::string desktop =
            "Default," +
            std::to_string(width) +
            "x" +
            std::to_string(height);

        command
            << "wine explorer "
            << shellQuote(
                "/desktop=" + desktop
            )
            << " "
            << shellQuote(executable);
    }
    else
    {
        command
            << "wine "
            << shellQuote(executable);
    }

    for(const auto& argument : ctx.arguments)
    {
        command
            << " "
            << shellQuote(argument);
    }
}


/*
    ================================================================
    BUILD WINE COMMAND
    ================================================================
*/

std::string buildWineCommand(
    const Context& ctx,
    const std::string&
)
{
    if(ctx.prefixMergedPfxDirectory.empty())
    {
        return {};
    }

    std::ostringstream command;

    /*
        ============================================================
        PREFIX
        ============================================================

        Wine must always use the merged overlay.

            LOWER:
                ~/.RetroDisc/prefix/<proton|wine>/<version>/
                    (contains pfx/, and for Proton also version and
                    tracked_files)

            UPPER:
                <game>/prefix
                    (mirrors the same layout: prefix/pfx/...)

            MERGED:
                /tmp/<gameId>-<pid>/merged_prefix
                    (the actual Wine prefix Wine/Proton use is the
                    "pfx" subdirectory of this merged tree)

        The persistent upper directory must NEVER be used directly
        as WINEPREFIX.
    */

    const auto prefix =
        ctx.prefixMergedPfxDirectory;

    const auto userDirectory =
        prefix /
        "drive_c" /
        "users" /
        CANONICAL_WINDOWS_USER;

    const auto localAppData =
        userDirectory /
        "AppData" /
        "Local";

    const auto xdgConfig =
        localAppData /
        "xdg-config";

    const auto xdgData =
        localAppData /
        "xdg-data";

    const auto xdgCache =
        localAppData /
        "xdg-cache";


    /*
        ============================================================
        PREFIX
        ============================================================
    */

    command
        << "export WINEPREFIX="
        << shellQuote(
            prefix.string()
        )
        << " && ";


    /*
        ============================================================
        WINDOWS USER
        ============================================================
    */

    command
        << "export USERNAME='RetroDisc'"
        << " && ";

    command
        << "export WINEUSERNAME='RetroDisc'"
        << " && ";

    command
        << "export USERPROFILE='C:\\users\\RetroDisc'"
        << " && ";

    command
        << "export APPDATA='C:\\users\\RetroDisc\\AppData\\Roaming'"
        << " && ";

    command
        << "export LOCALAPPDATA='C:\\users\\RetroDisc\\AppData\\Local'"
        << " && ";


    /*
        ============================================================
        XDG
        ============================================================
    */

    command
        << "mkdir -p "
        << shellQuote(
            xdgConfig.string()
        )
        << " "
        << shellQuote(
            xdgData.string()
        )
        << " "
        << shellQuote(
            xdgCache.string()
        )
        << " && ";

    command
        << "export XDG_CONFIG_HOME="
        << shellQuote(
            xdgConfig.string()
        )
        << " && ";

    command
        << "export XDG_DATA_HOME="
        << shellQuote(
            xdgData.string()
        )
        << " && ";

    command
        << "export XDG_CACHE_HOME="
        << shellQuote(
            xdgCache.string()
        )
        << " && ";


    /*
        ============================================================
        USER ENVIRONMENT
        ============================================================
    */

    if(!appendUserEnvironment(
        command,
        ctx
    ))
    {
        return {};
    }


    /*
        ============================================================
        WINE CONFIGURATION
        ============================================================
    */

    if(!appendWineConfiguration(
        command,
        ctx
    ))
    {
        return {};
    }


    /*
        ============================================================
        WORKING DIRECTORY
        ============================================================
    */

    command
        << "cd "
        << shellQuote(
            ctx.mergedDirectory.string()
        )
        << " && ";


    /*
        ============================================================
        GAME
        ============================================================
    */

    appendWineGameLaunch(
        command,
        ctx
    );

    return command.str();
}


std::string buildProtonCommand(
    const Context& ctx,
    const std::string&
)
{
    const auto proton =
        ctx.resolvedProtonPath.empty()
            ? findProton(ctx)
            : ctx.resolvedProtonPath;

    if(proton.empty())
    {
        std::cerr
            << "Proton not found.";

        if(!ctx.protonVersion.empty())
        {
            std::cerr
                << " "
                << ctx.protonVersion;
        }

        std::cerr
            << std::endl;

        return {};
    }


    /*
        ============================================================
        STEAM ROOT
        ============================================================

        Steam is optional.

        Proton may have been installed through:

            - Steam
            - ProtonUp-Qt
            - manual installation
            - another compatibility-tool manager

        If Steam exists, its root is passed to Proton through:

            STEAM_COMPAT_CLIENT_INSTALL_PATH

        If no Steam installation exists, Proton is still allowed
        to launch.
    */

    const auto steamRoot =
        findSteamRoot();

    if(steamRoot.empty())
    {
        std::cerr
            << "Steam installation not found."
            << std::endl;

        return {};
    }


    /*
        ============================================================
        PREFIX
        ============================================================
    */

    if(ctx.prefixMergedPfxDirectory.empty())
    {
        std::cerr
            << "Proton merged prefix is empty."
            << std::endl;

        return {};
    }

    const auto prefix =
        ctx.prefixMergedPfxDirectory;


    /*
        ============================================================
        PROTON COMPATIBILITY DATA
        ============================================================

        STEAM_COMPAT_DATA_PATH is simply the whole per-launch merged
        overlay:

            /tmp/<gameId>-<pid>/merged_prefix/
                version         (from the base build directory)
                tracked_files   (from the base build directory)
                pfx/            (== ctx.prefixMergedPfxDirectory)

        Confirmed by actual testing: Proton does NOT reliably honor
        WINEPREFIX for its own real prefix I/O -- it derives the
        prefix it actually operates on from
        $STEAM_COMPAT_DATA_PATH/pfx. Earlier versions of this code
        tried to work around that with a separate, synthetic
        compat-data directory whose "pfx" was a symlink to the
        merged overlay. That extra indirection turned out to make
        Proton re-run a full prefix setup on every launch (its own
        internal "is this compat-data already valid" checks did not
        consider a freshly rebuilt, mostly-empty synthetic directory
        equivalent to a real, complete compat-data directory),
        writing hundreds of MiB into <datapath>/prefix every single
        launch.

        The fix: ctx.prefixLowerDirectory is now the BUILD directory
        itself (~/.RetroDisc/prefix/proton/<Build>/, containing
        version, tracked_files AND pfx/) rather than just the pfx
        subdirectory, and <datapath>/prefix mirrors that same layout
        (<datapath>/prefix/pfx/... plus, if Proton ever legitimately
        changes them, <datapath>/prefix/version and
        <datapath>/prefix/tracked_files). STEAM_COMPAT_DATA_PATH and
        WINEPREFIX therefore resolve into the EXACT SAME overlay
        mount -- there is no separate synthetic directory to keep in
        sync, so Proton's own validity checks see a real, complete,
        already-initialized compat-data directory on every launch.
    */

    const auto compatData =
        ctx.prefixMergedDirectory;

    if(compatData.empty())
    {
        std::cerr
            << "Proton compat-data directory is not prepared."
            << std::endl;

        return {};
    }


    /*
        ============================================================
        PREFIX VALIDATION
        ============================================================
    */

    std::error_code ec;

    const auto prefixStatus =
        std::filesystem::symlink_status(
            prefix,
            ec
        );

    if(
        ec ||
        !std::filesystem::is_directory(prefixStatus)
    )
    {
        std::cerr
            << "Merged Wine prefix does not exist:"
            << std::endl
            << "    "
            << prefix
            << std::endl;

        return {};
    }


    std::ostringstream command;


    /*
        ============================================================
        PROTON COMPATIBILITY DATA
        ============================================================
    */

    if(!steamRoot.empty())
    {
        command
            << "export STEAM_COMPAT_CLIENT_INSTALL_PATH="
            << shellQuote(
                steamRoot.string()
            )
            << " && ";
    }

    command
        << "export STEAM_COMPAT_DATA_PATH="
        << shellQuote(
            compatData.string()
        )
        << " && ";


    /*
        ============================================================
        WINE PREFIX
        ============================================================

        WINEPREFIX and $STEAM_COMPAT_DATA_PATH/pfx are now the exact
        same directory (ctx.prefixMergedPfxDirectory), so it no
        longer matters which one Proton actually reads internally --
        both resolve to this launch's merged overlay.
    */

    command
        << "export WINEPREFIX="
        << shellQuote(
            prefix.string()
        )
        << " && ";


    /*
        ============================================================
        APP ID
        ============================================================
    */

    bool numericAppId =
        !ctx.gameId.empty();

    for(const char c : ctx.gameId)
    {
        if(c < '0' || c > '9')
        {
            numericAppId = false;
            break;
        }
    }

    if(numericAppId)
    {
        command
            << "export SteamAppId="
            << shellQuote(
                ctx.gameId
            )
            << " && ";

        command
            << "export SteamGameId="
            << shellQuote(
                ctx.gameId
            )
            << " && ";
    }
    else
    {
        command
            << "unset SteamAppId SteamGameId"
            << " 2>/dev/null"
            << " && ";
    }


    /*
        ============================================================
        WINDOWS USER
        ============================================================
    */

    command
        << "export USERNAME='RetroDisc'"
        << " && ";

    command
        << "export WINEUSERNAME='RetroDisc'"
        << " && ";

    command
        << "export USERPROFILE='C:\\users\\RetroDisc'"
        << " && ";

    command
        << "export APPDATA='C:\\users\\RetroDisc\\AppData\\Roaming'"
        << " && ";

    command
        << "export LOCALAPPDATA='C:\\users\\RetroDisc\\AppData\\Local'"
        << " && ";


    /*
        ============================================================
        XDG
        ============================================================
    */

    const auto userDirectory =
        prefix /
        "drive_c" /
        "users" /
        CANONICAL_WINDOWS_USER;

    const auto localAppData =
        userDirectory /
        "AppData" /
        "Local";

    const auto xdgConfig =
        localAppData /
        "xdg-config";

    const auto xdgData =
        localAppData /
        "xdg-data";

    const auto xdgCache =
        localAppData /
        "xdg-cache";


    command
        << "mkdir -p "
        << shellQuote(
            xdgConfig.string()
        )
        << " "
        << shellQuote(
            xdgData.string()
        )
        << " "
        << shellQuote(
            xdgCache.string()
        )
        << " && ";

    command
        << "export XDG_CONFIG_HOME="
        << shellQuote(
            xdgConfig.string()
        )
        << " && ";

    command
        << "export XDG_DATA_HOME="
        << shellQuote(
            xdgData.string()
        )
        << " && ";

    command
        << "export XDG_CACHE_HOME="
        << shellQuote(
            xdgCache.string()
        )
        << " && ";


    /*
        ============================================================
        USER ENVIRONMENT
        ============================================================
    */

    if(!appendUserEnvironment(
        command,
        ctx
    ))
    {
        return {};
    }


    /*
        ============================================================
        SYNC
        ============================================================
    */

    command
        << "export WINEESYNC="
        << shellQuote(
            ctx.wine.sync.esync
                ? "1"
                : "0"
        )
        << " && ";

    command
        << "export WINEFSYNC="
        << shellQuote(
            ctx.wine.sync.fsync
                ? "1"
                : "0"
        )
        << " && ";

    command
        << "export WINE_NTSYNC="
        << shellQuote(
            ctx.wine.sync.ntsync
                ? "1"
                : "0"
        )
        << " && ";


    /*
        ============================================================
        DLL OVERRIDES
        ============================================================
    */

    const std::string dllOverrides =
        buildDllOverrides(ctx);

    if(!dllOverrides.empty())
    {
        command
            << "export WINEDLLOVERRIDES="
            << shellQuote(
                dllOverrides
            )
            << " && ";
    }
    else
    {
        command
            << "unset WINEDLLOVERRIDES"
            << " 2>/dev/null"
            << " && ";
    }


    /*
        ============================================================
        WORKING DIRECTORY
        ============================================================
    */

    command
        << "cd "
        << shellQuote(
            ctx.mergedDirectory.string()
        )
        << " && ";


    /*
        ============================================================
        PROTON GAME
        ============================================================
    */

    command
        << shellQuote(
            proton.string()
        )
        << " run ";


    const auto executable =
        (
            ctx.mergedDirectory /
            ctx.executable
        ).string();

    if(ctx.wine.display.virtualDesktop.enabled)
    {
        const int width =
            ctx.wine.display.virtualDesktop.width > 0
                ? ctx.wine.display.virtualDesktop.width
                : 640;

        const int height =
            ctx.wine.display.virtualDesktop.height > 0
                ? ctx.wine.display.virtualDesktop.height
                : 480;

        const std::string desktop =
            "Default," +
            std::to_string(width) +
            "x" +
            std::to_string(height);

        command
            << "explorer "
            << shellQuote(
                "/desktop=" + desktop
            )
            << " "
            << shellQuote(
                executable
            );
    }
    else
    {
        command
            << shellQuote(
                executable
            );
    }

    for(const auto& argument : ctx.arguments)
    {
        command
            << " "
            << shellQuote(
                argument
            );
    }

    return command.str();
}


} // namespace

std::filesystem::path resolveProton(const Context& ctx)
{
    return findProton(ctx);
}

/*
    =================================================================
    PUBLIC LAUNCH FUNCTION
    =================================================================
*/

bool launchGame(
    Context& ctx
)
{
    if(!ctx.overlayMounted)
    {
        std::cerr
            << "Game overlay is not mounted."
            << std::endl;

        return false;
    }

    if(ctx.prefixMergedPfxDirectory.empty())
    {
        std::cerr
            << "Wine prefix path is empty."
            << std::endl;

        cleanupFilesystem(ctx);

        return false;
    }


    /*
        ============================================================
        GAME
        ============================================================
    */

    const auto gameDirectory =
        ctx.mergedDirectory;

    const auto executable =
        gameDirectory /
        ctx.executable;

    std::error_code ec;

    const auto gameStatus =
        std::filesystem::symlink_status(
            gameDirectory,
            ec
        );

    if(
        ec ||
        !std::filesystem::is_directory(gameStatus)
    )
    {
        std::cerr
            << "Merged game directory does not exist:"
            << std::endl
            << "    "
            << gameDirectory
            << std::endl;

        cleanupFilesystem(ctx);

        return false;
    }

    const auto executableStatus =
        std::filesystem::symlink_status(
            executable,
            ec
        );

    if(
        ec ||
        !(
            std::filesystem::is_regular_file(
                executableStatus
            ) ||
            std::filesystem::is_symlink(
                executableStatus
            )
        )
    )
    {
        std::cerr
            << "Executable not found:"
            << std::endl
            << "    "
            << executable
            << std::endl;

        cleanupFilesystem(ctx);

        return false;
    }


    /*
        ============================================================
        PREFIX
        ============================================================
    */

    const auto prefixDirectory =
        ctx.prefixMergedPfxDirectory;

    const auto prefixStatus =
        std::filesystem::symlink_status(
            prefixDirectory,
            ec
        );

    if(
        ec ||
        !std::filesystem::is_directory(prefixStatus)
    )
    {
        std::cerr
            << "Wine/Proton merged prefix not found:"
            << std::endl
            << "    "
            << prefixDirectory
            << std::endl;

        cleanupFilesystem(ctx);

        return false;
    }


    /*
        ============================================================
        RUNTIME USER
        ============================================================
    */

    const std::string runtimeUser =
        determineRuntimeUser(ctx);

    if(runtimeUser != CANONICAL_WINDOWS_USER)
    {
        std::cerr
            << "Invalid RetroDisc runtime user."
            << std::endl;

        cleanupFilesystem(ctx);

        return false;
    }

    /*
        This is deliberately performed for BOTH Wine and Proton.

        The canonical profile is always:

            drive_c/users/RetroDisc

        Broken legacy/canonical symlinks are repaired before the
        runtime is started.
    */

    if(!prepareRuntimeUser(
        ctx,
        runtimeUser
    ))
    {
        cleanupFilesystem(ctx);

        return false;
    }


    /*
        ============================================================
        CONFIGURATION
        ============================================================
    */

    std::cout
        << std::endl
        << "================================================"
        << std::endl
        << "WINE / PROTON CONFIGURATION"
        << std::endl
        << "================================================"
        << std::endl;

    std::cout
        << "Merged prefix:"
        << std::endl
        << "    "
        << prefixDirectory
        << std::endl;

    std::cout
        << "Runtime:"
        << std::endl
        << "    "
        << ctx.runtime
        << std::endl;

    std::cout
        << "Windows user:"
        << std::endl
        << "    "
        << CANONICAL_WINDOWS_USER
        << std::endl;

    std::cout
        << "Canonical Windows profile:"
        << std::endl
        << "    "
        << (
            prefixDirectory /
            "drive_c" /
            "users" /
            CANONICAL_WINDOWS_USER
        )
        << std::endl;

    std::cout
        << "Windows version:"
        << std::endl
        << "    "
        << (
            ctx.wine.windowsVersion.empty()
                ? "unchanged"
                : ctx.wine.windowsVersion
        )
        << std::endl;

    std::cout
        << "Renderer:"
        << std::endl
        << "    "
        << (
            ctx.wine.graphics.renderer.empty()
                ? "auto"
                : ctx.wine.graphics.renderer
        )
        << std::endl;

    std::cout
        << "Video memory:"
        << std::endl
        << "    "
        << ctx.wine.graphics.videoMemory
        << " MB"
        << std::endl;

    std::cout
        << "Strict draw ordering:"
        << std::endl
        << "    "
        << (
            ctx.wine.graphics.strictDrawOrdering
                ? "enabled"
                : "disabled"
        )
        << std::endl;

    std::cout
        << "Virtual desktop:"
        << std::endl
        << "    "
        << (
            ctx.wine.display.virtualDesktop.enabled
                ? "enabled"
                : "disabled"
        )
        << std::endl;

    if(ctx.wine.display.virtualDesktop.enabled)
    {
        std::cout
            << "Virtual desktop size:"
            << std::endl
            << "    "
            << ctx.wine.display.virtualDesktop.width
            << "x"
            << ctx.wine.display.virtualDesktop.height
            << std::endl;
    }

    std::cout
        << "Window managed:"
        << std::endl
        << "    "
        << (
            ctx.wine.display.window.managed
                ? "yes"
                : "no"
        )
        << std::endl;

    std::cout
        << "Window decorations:"
        << std::endl
        << "    "
        << (
            ctx.wine.display.window.decorations
                ? "yes"
                : "no"
        )
        << std::endl;

    std::cout
        << "Mouse capture:"
        << std::endl
        << "    "
        << (
            ctx.wine.display.window.mouseCapture
                ? "yes"
                : "no"
        )
        << std::endl;

    std::cout
        << "DPI:"
        << std::endl
        << "    "
        << ctx.wine.display.dpi
        << std::endl;

    std::cout
        << "ESYNC:"
        << std::endl
        << "    "
        << (
            ctx.wine.sync.esync
                ? "enabled"
                : "disabled"
        )
        << std::endl;

    std::cout
        << "FSYNC:"
        << std::endl
        << "    "
        << (
            ctx.wine.sync.fsync
                ? "enabled"
                : "disabled"
        )
        << std::endl;

    std::cout
        << "NTSYNC:"
        << std::endl
        << "    "
        << (
            ctx.wine.sync.ntsync
                ? "enabled"
                : "disabled"
        )
        << std::endl;


    /*
        ============================================================
        LAUNCH
        ============================================================
    */

    std::cout
        << std::endl
        << "Launching "
        << ctx.runtime
        << "..."
        << std::endl;

    std::cout
        << "Merged Wine/Proton prefix:"
        << std::endl
        << "    "
        << prefixDirectory
        << std::endl;

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
        << "Linux user is NOT used as Windows user."
        << std::endl;

    printEnvironment(ctx);


    /*
        ============================================================
        BUILD COMMAND
        ============================================================
    */

    std::string command;

    if(ctx.runtime == "proton")
    {
        command =
            buildProtonCommand(
                ctx,
                runtimeUser
            );
    }
    else
    {
        command =
            buildWineCommand(
                ctx,
                runtimeUser
            );
    }

    if(command.empty())
    {
        cleanupRuntimeUser(
            ctx,
            runtimeUser
        );

        cleanupFilesystem(ctx);

        return false;
    }


    /*
        ============================================================
        EXECUTE
        ============================================================
    */

    const int result =
        runCommand(command);


    /*
        ============================================================
        CLEANUP
        ============================================================
    */

    cleanupRuntimeUser(
        ctx,
        runtimeUser
    );

    const bool cleanupResult =
        cleanupFilesystem(ctx);

    if(result == -1)
    {
        return false;
    }

    if(!WIFEXITED(result))
    {
        return false;
    }

    const int exitCode =
        WEXITSTATUS(result);

    if(!cleanupResult)
    {
        return false;
    }

    if(exitCode != 0)
    {
        std::cout
            << "Game exited with code "
            << exitCode
            << "."
            << std::endl;

        return false;
    }

    return true;
}