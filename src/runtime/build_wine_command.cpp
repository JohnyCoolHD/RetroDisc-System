#include "runtime_internal.hpp"
#include "context.hpp"

#include <sstream>
#include <string>


namespace runtime_internal
{


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


} // namespace runtime_internal
