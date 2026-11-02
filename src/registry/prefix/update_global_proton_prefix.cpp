#include "prefix_internal.hpp"
#include "../registry_internal.hpp"
#include "context.hpp"
#include "../../filesystem/filesystem_internal.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <system_error>


namespace prefix_internal
{


/*
    ================================================================
    READ TRIMMED FILE
    ================================================================
*/

static std::string readTrimmedFile(
    const std::filesystem::path& path
)
{
    std::ifstream file(
        path
    );

    if(!file)
    {
        return {};
    }

    std::ostringstream content;

    content << file.rdbuf();

    std::string text =
        content.str();

    while(
        !text.empty() &&
        (
            text.back() == '\n' ||
            text.back() == '\r' ||
            text.back() == ' '
        )
    )
    {
        text.pop_back();
    }

    return text;
}


/*
    ================================================================
    UPDATE GLOBAL PROTON PREFIX

    Steam updates "Proton - Experimental" in place. The global
    prefix keeps the same folder name, but its Proton version is
    then outdated. If the game was started like that, Proton would
    upgrade the prefix inside the game overlay and the complete
    upgraded prefix would end up in <datapath>.

    So the global prefix is upgraded here, before any overlay
    is mounted.
    ================================================================
*/

bool updateGlobalProtonPrefix(
    Context& ctx
)
{
    const auto protonDirectory =
        ctx.resolvedProtonPath.parent_path();

    const auto markerFile =
        ctx.globalPrefixDirectory /
        ".retrodisc_proton_version";


    std::string currentVersion =
        readTrimmedFile(
            protonDirectory /
            "version"
        );


    if(currentVersion.empty())
    {
        std::error_code timeError;

        const auto writeTime =
            std::filesystem::last_write_time(
                ctx.resolvedProtonPath,
                timeError
            );

        if(timeError)
        {
            std::cerr
                << "Could not determine the Proton version:"
                << std::endl
                << "    "
                << ctx.resolvedProtonPath
                << std::endl;

            return false;
        }

        currentVersion =
            "mtime-" +
            std::to_string(
                writeTime
                    .time_since_epoch()
                    .count()
            );
    }


    if(readTrimmedFile(markerFile) == currentVersion)
    {
        return true;
    }


    std::cout
        << "Proton changed since the global base prefix was "
        << "last used. Updating global base prefix..."
        << std::endl;


    const auto steamInstallPath =
        findSteamRootForProton(
            ctx.resolvedProtonPath
        );

    if(steamInstallPath.empty())
    {
        std::cerr
            << "Could not determine Steam root for Proton:"
            << std::endl
            << "    "
            << ctx.resolvedProtonPath
            << std::endl;

        return false;
    }


    std::ostringstream command;

    command
        << "STEAM_COMPAT_CLIENT_INSTALL_PATH="
        << shellQuote(
            steamInstallPath.string()
        )
        << " "
        << "STEAM_COMPAT_DATA_PATH="
        << shellQuote(
            ctx.globalPrefixDirectory.string()
        )
        << " "
        << "WINEPREFIX="
        << shellQuote(
            (ctx.globalPrefixDirectory / "pfx").string()
        )
        << " "
        << shellQuote(
            ctx.resolvedProtonPath.string()
        )
        << " run "
        << shellQuote(
            "wineboot"
        )
        << " -u";


    if(!runCommand(
        command.str(),
        true
    ))
    {
        std::cerr
            << "Could not update global base prefix."
            << std::endl;

        return false;
    }


    const std::string waitCommand =
        "WINEPREFIX=" +
        shellQuote(
            (ctx.globalPrefixDirectory / "pfx").string()
        ) +
        " wineserver -w";

    if(!runCommand(
        waitCommand,
        true
    ))
    {
        std::cerr
            << "Could not wait for Wine server after prefix update."
            << std::endl;

        return false;
    }


    if(!prefixLooksValid(
        ctx.globalPrefixDirectory
    ))
    {
        std::cerr
            << "Global base prefix is incomplete after the update:"
            << std::endl
            << "    "
            << ctx.globalPrefixDirectory
            << std::endl;

        return false;
    }


    {
        std::ofstream marker(
            markerFile,
            std::ios::trunc
        );

        if(!marker)
        {
            std::cerr
                << "Could not write prefix version marker:"
                << std::endl
                << "    "
                << markerFile
                << std::endl;

            return false;
        }

        marker
            << currentVersion
            << std::endl;
    }


    std::cout
        << "Global base prefix updated."
        << std::endl;


    return true;
}


} // namespace prefix_internal
