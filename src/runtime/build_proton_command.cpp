#include "runtime_internal.hpp"
#include "context.hpp"

#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <system_error>


namespace runtime_internal
{


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


} // namespace runtime_internal
