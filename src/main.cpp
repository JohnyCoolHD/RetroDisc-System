#include <filesystem>
#include <iostream>
#include <string>

#include <sys/prctl.h>

#include "context.hpp"
#include "config.hpp"
#include "filesystem.hpp"
#include "runtime.hpp"
#include "registry.hpp"
#include "discord.hpp"


namespace
{


/*
    ================================================================
    DISCORD APPLICATION
    ================================================================
*/

constexpr const char* DISCORD_APPLICATION_ID =
    "1536835815546822716";


/*
    ================================================================
    LINUX PROCESS NAME
    ================================================================
*/

void setProcessName(
    const std::string& gameName
)
{
    std::string processName =
        gameName.substr(
            0,
            15
        );


    if(processName.empty())
    {
        processName =
            "RetroDisc";
    }


    prctl(
        PR_SET_NAME,
        processName.c_str(),
        0,
        0,
        0
    );
}


}


int main(
    int argc,
    char* argv[]
)
{
    Context ctx;


    /*
        ============================================================
        DISCORD
        ============================================================
    */

    DiscordPresence discord;


    /*
        ============================================================
        VALIDATE ARGUMENTS
        ============================================================
    */

    if(
        argc < 1 ||
        argv == nullptr ||
        argv[0] == nullptr
    )
    {
        std::cerr
            << "Could not determine RetroDisc executable path."
            << std::endl;

        return 1;
    }


    /*
        ============================================================
        DETERMINE RETRODISC ROOT
        ============================================================
    */

    try
    {
        ctx.root =
            std::filesystem::canonical(
                argv[0]
            ).parent_path();
    }
    catch(const std::exception& e)
    {
        std::cerr
            << "Could not determine executable directory:"
            << std::endl
            << e.what()
            << std::endl;


        try
        {
            ctx.root =
                std::filesystem::current_path();
        }
        catch(...)
        {
            return 1;
        }
    }


    std::cout
        << "RetroDisc Root: "
        << ctx.root
        << std::endl;


    /*
        ============================================================
        GAMEDATA
        ============================================================
    */

    if(
        !std::filesystem::is_directory(
            ctx.root /
            "gamedata"
        )
    )
    {
        std::cerr
            << "GameData directory was not found:"
            << std::endl
            << "    "
            << ctx.root /
               "gamedata"
            << std::endl;

        return 1;
    }


    /*
        ============================================================
        MANIFEST
        ============================================================
    */

    if(
        !loadManifest(
            ctx
        )
    )
    {
        std::cerr
            << "Failed to load manifest."
            << std::endl;

        return 1;
    }


    /*
        ============================================================
        PROCESS NAME
        ============================================================
    */

    setProcessName(
        ctx.gameName
    );


    /*
        ============================================================
        CONFIG
        ============================================================

        loadConfig() automatically creates:

            ~/.config/RetroDisc/games/<gameId>/config.json

        when it does not exist.

        The embedded runtime configuration is selected using
        ctx.runtime, which came from manifest.json.
    */

    if(
        !loadConfig(
            ctx
        )
    )
    {
        std::cerr
            << "Failed to load config."
            << std::endl;

        return 1;
    }


    /*
        ============================================================
        GAME FILESYSTEM
        ============================================================
    */

    if(
        !prepareFilesystem(
            ctx
        )
    )
    {
        std::cerr
            << "Filesystem preparation failed."
            << std::endl;

        cleanup(
            ctx
        );

        return 1;
    }


    /*
        ============================================================
        WINE PREFIX
        ============================================================
    */

    if(
        !preparePrefix(
            ctx
        )
    )
    {
        std::cerr
            << "Prefix preparation failed."
            << std::endl;

        cleanup(
            ctx
        );

        return 1;
    }


    /*
        ============================================================
        DISCORD RICH PRESENCE
        ============================================================
    */

    if(
        discord.connect(
            DISCORD_APPLICATION_ID
        )
    )
    {
        discord.setActivity(
            ctx.gameName,
            ctx.gameId
        );
    }


    /*
        ============================================================
        LAUNCH
        ============================================================
    */

    if(
        !launchGame(
            ctx
        )
    )
    {
        std::cerr
            << "Game launch failed."
            << std::endl;

        discord.clearActivity();

        cleanup(
            ctx
        );

        return 1;
    }


    /*
        ============================================================
        DISCORD CLEANUP
        ============================================================
    */

    discord.clearActivity();


    /*
        ============================================================
        FINAL CLEANUP
        ============================================================
    */

    if(
        !cleanup(
            ctx
        )
    )
    {
        return 1;
    }


    return 0;
}