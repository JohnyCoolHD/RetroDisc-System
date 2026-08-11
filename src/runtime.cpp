#include "runtime.hpp"
#include "filesystem.hpp"
#include "discord.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <system_error>

#include <sys/wait.h>
#include <unistd.h>


namespace
{

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
    HOME
    ================================================================
*/

std::filesystem::path getHome()
{
    const char* home =
        std::getenv("HOME");

    if(home == nullptr)
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
    const auto home = getHome();

    if(home.empty())
    {
        return {};
    }

    const std::vector<std::filesystem::path> roots =
    {
        home /
        ".local" /
        "share" /
        "Steam",

        home /
        ".steam" /
        "root",

        home /
        ".steam" /
        "steam"
    };

    for(const auto& root : roots)
    {
        std::error_code ec;

        if(
            std::filesystem::is_directory(
                root /
                "steamapps",
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
    const auto home = getHome();

    if(home.empty())
    {
        return {};
    }

    /*
        Explicit Proton path has priority.
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
    }

    if(ctx.protonVersion.empty())
    {
        return {};
    }

    const std::vector<std::filesystem::path> roots =
    {
        home /
        ".local" /
        "share" /
        "Steam",

        home /
        ".steam" /
        "root",

        home /
        ".steam" /
        "steam"
    };

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

    return {};
}


/*
    ================================================================
    ENVIRONMENT VARIABLE NAME
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

    const char first = name[0];

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

    for(
        std::size_t i = 1;
        i < name.size();
        ++i
    )
    {
        const char c = name[i];

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
    ENVIRONMENT
    ================================================================
*/

void printEnvironment(
    const Context& ctx
)
{
    for(
        const auto& [key, value] :
        ctx.environment
    )
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

    for(
        const auto& [dll, overrideValue] :
        ctx.wine.dllOverrides
    )
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
    COMMON USER ENVIRONMENT
    ================================================================
*/

bool appendUserEnvironment(
    std::ostringstream& command,
    const Context& ctx
)
{
    for(
        const auto& [key, value] :
        ctx.environment
    )
    {
        /*
            Runtime-owned variables must never be overridden
            by the game configuration.
        */

        if(
            key == "WINEPREFIX" ||
            key == "STEAM_COMPAT_DATA_PATH" ||
            key == "STEAM_COMPAT_CLIENT_INSTALL_PATH" ||
            key == "STEAM_COMPAT_INSTALL_PATH" ||
            key == "WINEUSERNAME" ||
            key == "USERNAME" ||
            key == "USERPROFILE" ||
            key == "APPDATA" ||
            key == "LOCALAPPDATA"
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
    DETERMINE RUNTIME USER
    ================================================================

    Wine:

        Linux USER / LOGNAME

    Proton:

        Steam normally supplies:
            SteamUser
            Steam username information

        If none is available, fall back to USER.

    The name is NEVER hardcoded.

    RetroDisc itself remains the persistent profile.
*/

std::string determineRuntimeUser(
    const Context& ctx
)
{
    const char* candidates[] =
    {
        std::getenv("STEAM_USER"),
        std::getenv("SteamUser"),
        std::getenv("USER"),
        std::getenv("LOGNAME")
    };

    for(const char* candidate : candidates)
    {
        if(
            candidate != nullptr &&
            *candidate != '\0'
        )
        {
            std::string value(candidate);

            /*
                Wine/Windows usernames should not contain
                path separators.

                If an invalid value is supplied, simply
                continue to the next candidate.
            */

            if(
                value != "." &&
                value != ".." &&
                value.find('/') == std::string::npos &&
                value.find('\\') == std::string::npos
            )
            {
                return value;
            }
        }
    }

    return {};
}


/*
    ================================================================
    TEMPORARY USER PROFILE
    ================================================================

    Persistent:

        drive_c/users/RetroDisc

    Runtime:

        drive_c/users/<real user>

    The runtime user is a symlink to RetroDisc.

    Example:

        drive_c/users/
            RetroDisc/
            maxim -> RetroDisc

    or:

        drive_c/users/
            RetroDisc/
            steamuser -> RetroDisc

    Therefore:

        %USERPROFILE%
        %APPDATA%
        %LOCALAPPDATA%
        Documents
        Desktop
        Saved Games
        etc.

    all physically end up inside RetroDisc.
*/

bool prepareRuntimeUser(
    const Context& ctx,
    const std::string& runtimeUser
)
{
    if(
        ctx.prefixOverlayDirectory.empty() ||
        runtimeUser.empty()
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
        "RetroDisc";

    const auto runtimeUserDirectory =
        usersDirectory /
        runtimeUser;

    std::error_code ec;

    if(
        !std::filesystem::is_directory(
            usersDirectory,
            ec
        )
    )
    {
        std::cerr
            << "Wine users directory does not exist:"
            << std::endl
            << "    "
            << usersDirectory
            << std::endl;

        return false;
    }

    if(
        !std::filesystem::is_directory(
            persistentUser,
            ec
        )
    )
    {
        std::cerr
            << "Persistent RetroDisc user does not exist:"
            << std::endl
            << "    "
            << persistentUser
            << std::endl;

        return false;
    }

    /*
        RetroDisc itself must never be replaced.
    */

    if(runtimeUser == "RetroDisc")
    {
        std::cerr
            << "Runtime user unexpectedly equals RetroDisc."
            << std::endl;

        return false;
    }

    /*
        Remove an old runtime-user link/directory.

        Normally this should not exist because cleanup runs
        after every game. This also makes crashed previous
        launches recoverable.
    */

    if(
        std::filesystem::exists(
            runtimeUserDirectory,
            ec
        ) ||
        std::filesystem::is_symlink(
            runtimeUserDirectory,
            ec
        )
    )
    {
        ec.clear();

        /*
            We only remove the runtime user if it is a symlink.
            Never recursively delete a real user directory.
        */

        const auto status =
            std::filesystem::symlink_status(
                runtimeUserDirectory,
                ec
            );

        if(ec)
        {
            std::cerr
                << "Could not inspect runtime user:"
                << std::endl
                << "    "
                << runtimeUserDirectory
                << std::endl;

            return false;
        }

        if(std::filesystem::is_symlink(status))
        {
            std::filesystem::remove(
                runtimeUserDirectory,
                ec
            );

            if(ec)
            {
                std::cerr
                    << "Could not remove old runtime user link:"
                    << std::endl
                    << "    "
                    << runtimeUserDirectory
                    << std::endl
                    << "    "
                    << ec.message()
                    << std::endl;

                return false;
            }
        }
        else
        {
            std::cerr
                << "Runtime user directory already exists and is"
                << " not a symlink:"
                << std::endl
                << "    "
                << runtimeUserDirectory
                << std::endl
                << "Refusing to delete it."
                << std::endl;

            return false;
        }
    }

    /*
        Create:

            <runtimeUser> -> RetroDisc
    */

    std::filesystem::create_symlink(
        "RetroDisc",
        runtimeUserDirectory,
        ec
    );

    if(ec)
    {
        std::cerr
            << "Could not create runtime user link:"
            << std::endl
            << "    "
            << runtimeUserDirectory
            << std::endl
            << " -> RetroDisc"
            << std::endl
            << "    "
            << ec.message()
            << std::endl;

        return false;
    }

    std::cout
        << "Runtime user:"
        << std::endl
        << "    "
        << runtimeUser
        << std::endl;

    std::cout
        << "Persistent user:"
        << std::endl
        << "    RetroDisc"
        << std::endl;

    std::cout
        << "Runtime profile:"
        << std::endl
        << "    "
        << runtimeUserDirectory
        << std::endl;

    std::cout
        << "Persistent profile:"
        << std::endl
        << "    "
        << persistentUser
        << std::endl;

    return true;
}


/*
    ================================================================
    REMOVE TEMPORARY USER
    ================================================================
*/

void cleanupRuntimeUser(
    const Context& ctx,
    const std::string& runtimeUser
)
{
    if(
        ctx.prefixOverlayDirectory.empty() ||
        runtimeUser.empty() ||
        runtimeUser == "RetroDisc"
    )
    {
        return;
    }

    const auto runtimeUserDirectory =
        ctx.prefixOverlayDirectory /
        "drive_c" /
        "users" /
        runtimeUser;

    std::error_code ec;

    const auto status =
        std::filesystem::symlink_status(
            runtimeUserDirectory,
            ec
        );

    if(ec)
    {
        return;
    }

    /*
        Only remove our symlink.

        Never recursively remove an actual directory.
    */

    if(
        std::filesystem::is_symlink(
            status
        )
    )
    {
        std::filesystem::remove(
            runtimeUserDirectory,
            ec
        );

        if(!ec)
        {
            std::cout
                << "Temporary runtime user removed:"
                << std::endl
                << "    "
                << runtimeUser
                << std::endl;
        }
        else
        {
            std::cerr
                << "Could not remove temporary runtime user:"
                << std::endl
                << "    "
                << runtimeUserDirectory
                << std::endl
                << "    "
                << ec.message()
                << std::endl;
        }
    }
}


/*
    ================================================================
    WINE COMMAND
    ================================================================
*/

std::string buildWineCommand(
    const Context& ctx,
    const std::string& runtimeUser
)
{
    if(ctx.prefixOverlayDirectory.empty())
    {
        return {};
    }

    std::ostringstream command;

    /*
        Canonical Wine prefix.
    */

    command
        << "export WINEPREFIX="
        << shellQuote(
            ctx.prefixOverlayDirectory.string()
        )
        << " && ";

    /*
        Runtime username.

        Wine uses this for Windows-side user information.
    */

    command
        << "export USERNAME="
        << shellQuote(
            runtimeUser
        )
        << " && ";

    command
        << "export WINEUSERNAME="
        << shellQuote(
            runtimeUser
        )
        << " && ";

    /*
        User environment.
    */

    if(!appendUserEnvironment(
        command,
        ctx
    ))
    {
        return {};
    }

    /*
        DLL overrides.

        IMPORTANT:
        Keep this variable inside the function scope.
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
        Working directory.
    */

    command
        << "cd "
        << shellQuote(
            ctx.mergedDirectory.string()
        )
        << " && ";

    /*
        Wine executable.
    */

    command
        << "wine "
        << shellQuote(
            (
                ctx.mergedDirectory /
                ctx.executable
            ).string()
        );

    /*
        Arguments.
    */

    for(
        const auto& argument :
        ctx.arguments
    )
    {
        command
            << " "
            << shellQuote(
                argument
            );
    }

    return command.str();
}


/*
    ================================================================
    PROTON COMMAND
    ================================================================
*/

std::string buildProtonCommand(
    const Context& ctx,
    const std::string& runtimeUser
)
{
    const auto proton =
        findProton(ctx);

    if(proton.empty())
    {
        std::cerr
            << "Proton not found:"
            << std::endl
            << "    "
            << ctx.protonVersion
            << std::endl;

        return {};
    }

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
        Proton compatibility data:

            ~/Games/RetroDisc/<gameId>/

        Prefix:

            ~/Games/RetroDisc/<gameId>/pfx/
    */

    const auto compatData =
        ctx.prefixOverlayDirectory.parent_path();

    if(compatData.empty())
    {
        std::cerr
            << "Could not determine Proton compatibility-data directory."
            << std::endl;

        return {};
    }

    const auto sharedPrefix =
        ctx.prefixOverlayDirectory;

    std::error_code ec;

    if(
        !std::filesystem::is_directory(
            sharedPrefix,
            ec
        )
    )
    {
        std::cerr
            << "Shared Wine/Proton prefix does not exist:"
            << std::endl
            << "    "
            << sharedPrefix
            << std::endl;

        return {};
    }

    std::ostringstream command;

    /*
        ============================================================
        STEAM / PROTON ENVIRONMENT
        ============================================================
    */

    command
        << "export STEAM_COMPAT_CLIENT_INSTALL_PATH="
        << shellQuote(
            steamRoot.string()
        )
        << " && ";

    command
        << "export STEAM_COMPAT_DATA_PATH="
        << shellQuote(
            compatData.string()
        )
        << " && ";

    command
        << "export STEAM_COMPAT_INSTALL_PATH="
        << shellQuote(
            ctx.mergedDirectory.string()
        )
        << " && ";

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

    /*
        ============================================================
        RUNTIME USER
        ============================================================
    */

    command
        << "export USERNAME="
        << shellQuote(
            runtimeUser
        )
        << " && ";

    command
        << "export WINEUSERNAME="
        << shellQuote(
            runtimeUser
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
        PROTON
        ============================================================
    */

    command
        << shellQuote(
            proton.string()
        )
        << " run "
        << shellQuote(
            (
                ctx.mergedDirectory /
                ctx.executable
            ).string()
        );

    /*
        Arguments.
    */

    for(
        const auto& argument :
        ctx.arguments
    )
    {
        command
            << " "
            << shellQuote(
                argument
            );
    }

    return command.str();
}


/*
    ================================================================
    RUN COMMAND
    ================================================================
*/

int runCommand(
    const std::string& command
)
{
    std::cout
        << "Runtime command:"
        << std::endl
        << command
        << std::endl;

    return std::system(
        command.c_str()
    );
}

} // namespace


/*
    ================================================================
    LAUNCH GAME
    ================================================================
*/

bool launchGame(
    Context& ctx
)
{
    if(!ctx.overlayMounted)
    {
        return false;
    }

    if(
        ctx.prefixOverlayDirectory.empty()
    )
    {
        cleanupFilesystem(ctx);
        return false;
    }

    const auto gameDirectory =
        ctx.mergedDirectory;

    const auto executable =
        gameDirectory /
        ctx.executable;

    /*
        ============================================================
        GAME VALIDATION
        ============================================================
    */

    if(
        !std::filesystem::is_directory(
            gameDirectory
        )
    )
    {
        cleanupFilesystem(ctx);
        return false;
    }

    if(
        !std::filesystem::exists(
            executable
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
        SHARED PREFIX VALIDATION
        ============================================================
    */

    if(
        !std::filesystem::is_directory(
            ctx.prefixOverlayDirectory
        )
    )
    {
        std::cerr
            << "Shared Wine/Proton prefix not found:"
            << std::endl
            << "    "
            << ctx.prefixOverlayDirectory
            << std::endl;

        cleanupFilesystem(ctx);
        return false;
    }

    /*
        ============================================================
        DETERMINE TEMPORARY RUNTIME USER
        ============================================================
    */

    const std::string runtimeUser =
        determineRuntimeUser(ctx);

    if(runtimeUser.empty())
    {
        std::cerr
            << "Could not determine runtime user."
            << std::endl;

        cleanupFilesystem(ctx);
        return false;
    }

    /*
        Create:

            drive_c/users/<runtimeUser>
                -> RetroDisc
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
        << "Shared prefix:"
        << std::endl
        << "    "
        << ctx.prefixOverlayDirectory
        << std::endl;

    std::cout
        << "Runtime user:"
        << std::endl
        << "    "
        << runtimeUser
        << std::endl;

    std::cout
        << "Persistent user:"
        << std::endl
        << "    RetroDisc"
        << std::endl;

    printEnvironment(ctx);

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
        RUN
        ============================================================
    */

    const int result =
        runCommand(
            command
        );

    /*
        ============================================================
        REMOVE TEMPORARY USER
        ============================================================
    */

    cleanupRuntimeUser(
        ctx,
        runtimeUser
    );

    /*
        ============================================================
        FILESYSTEM CLEANUP
        ============================================================
    */

    const bool cleanupResult =
        cleanupFilesystem(
            ctx
        );

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
