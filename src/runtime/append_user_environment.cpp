#include "runtime_internal.hpp"
#include "context.hpp"

#include <iostream>
#include <sstream>


namespace runtime_internal
{


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


} // namespace runtime_internal
