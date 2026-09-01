#include "config_internal.hpp"

#include <cstdlib>


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

    if(*home == '\0')
    {
        return {};
    }

    return std::filesystem::path(home);
}


/*
    ================================================================
    CONFIGURATION DIRECTORY
    ================================================================
*/

std::filesystem::path getGameConfigDirectory(
    const Context& ctx
)
{
    /*
        ============================================================
        EXPLICIT DATAPATH
        ============================================================

        With --datapath the supplied directory is the
        complete persistent game directory.

        Example:

            --datapath /media/MEMORYCARD/resident-evil-1

        Config:

            /media/MEMORYCARD/resident-evil-1/config.json
    */

    if(!ctx.dataPath.empty())
    {
        return ctx.dataPath;
    }


    /*
        ============================================================
        DEFAULT GAME DIRECTORY
        ============================================================

        Without --datapath:

            ~/Games/RetroDisc/<gameId>/

        Config:

            ~/Games/RetroDisc/<gameId>/config.json
    */

    const auto home =
        getHome();

    if(home.empty())
    {
        return {};
    }

    return
        home /
        "Games" /
        "RetroDisc" /
        ctx.gameId;
}


/*
    ================================================================
    CONFIGURATION PATH
    ================================================================
*/

std::filesystem::path getGameConfigPath(
    const Context& ctx
)
{
    const auto directory =
        getGameConfigDirectory(ctx);

    if(directory.empty())
    {
        return {};
    }

    return
        directory /
        "config.json";
}