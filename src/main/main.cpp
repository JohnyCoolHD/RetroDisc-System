#include "main_internal.hpp"
#include "context.hpp"
#include "config.hpp"
#include "filesystem.hpp"
#include "runtime.hpp"
#include "registry.hpp"
#include "discord.hpp"

#include <exception>
#include <filesystem>
#include <iostream>


using namespace main_internal;


/*
    ================================================================
    MAIN
    ================================================================
*/

int main(
    int argc,
    char* argv[]
)
{
    Context ctx;

    DiscordPresence discord;


    /*
        ============================================================
        VALIDATE ARGUMENTS
        ============================================================
    */

    if(
        argc < 1 ||
        argv == nullptr ||
        argv[0] == nullptr ||
        *argv[0] == '\0'
    )
    {
        std::cerr
            << "Could not determine RetroDisc executable path."
            << std::endl;

        return 1;
    }


    /*
        ============================================================
        COMMAND LINE
        ============================================================
    */

    if(!parseCommandLine(
        argc,
        argv,
        ctx
    ))
    {
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
        DATA PATH
        ============================================================
    */

    if(!ctx.dataPath.empty())
    {
        std::cout
            << "Custom DataPath:"
            << std::endl
            << "    "
            << ctx.dataPath
            << std::endl;
    }
    else
    {
        std::cout
            << "DataPath:"
            << std::endl
            << "    Default"
            << std::endl;
    }


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
        /*
            Discord failure must never prevent the game from
            launching.
        */

        if(
            !discord.setActivity(
                ctx.gameName,
                ctx.gameId
            )
        )
        {
            discord.disconnect();
        }
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
        discord.disconnect();

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
    discord.disconnect();


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
