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
        ctx.prefixOverlayDirectory /
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

    /*
        IMPORTANT:

        Do NOT create:

            users/maxim
            users/steamuser
            users/Public

        as active canonical profiles.

        RetroDisc is the one persistent Windows user.

        Existing legacy profiles may remain on disk for backwards
        compatibility, but Wine/Proton is always pointed at the
        RetroDisc profile.
    */

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
        ctx.prefixOverlayDirectory /
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
    if(ctx.prefixMergedDirectory.empty())
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
                ~/.RetroDisc/pfx

            UPPER:
                <game>/pfx

            MERGED:
                /tmp/RetroDisc-<gameId>/merged

        The persistent upper directory must NEVER be used directly
        as WINEPREFIX.
    */

    const auto prefix =
        ctx.prefixMergedDirectory;

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
        findProton(ctx);

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

    if(ctx.prefixMergedDirectory.empty())
    {
        std::cerr
            << "Proton merged prefix is empty."
            << std::endl;

        return {};
    }

    const auto prefix =
        ctx.prefixMergedDirectory;


    /*
        ============================================================
        PROTON COMPATIBILITY DATA
        ============================================================

        The compatibility-data directory is still the persistent
        per-game directory:

            <gameDirectory>/
                pfx/       <- persistent overlay UPPER
                ...

        Proton gets this directory through:

            STEAM_COMPAT_DATA_PATH

        But the actual Wine prefix is the mounted overlay:

            /tmp/RetroDisc-<gameId>/merged

        Therefore WINEPREFIX explicitly points to the merged
        overlay and MUST NOT point to the persistent upperdir.
    */

    const auto compatData =
        ctx.prefixMergedDirectory.parent_path();

    if(compatData.empty())
    {
        std::cerr
            << "Could not determine Proton compatibility-data"
            << " directory."
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

        Proton must use the merged overlay as its actual prefix.

        Do NOT let Proton fall back to:

            STEAM_COMPAT_DATA_PATH/pfx
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

    if(ctx.prefixMergedDirectory.empty())
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
        ctx.prefixMergedDirectory;

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
        << "Shared prefix:"
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
        << "Shared Wine/Proton prefix:"
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