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
    const auto home =
        getHome();

    if(home.empty())
    {
        return {};
    }

    return
        home /
        ".config" /
        "RetroDisc" /
        "games" /
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