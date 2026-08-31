#include "registry_internal.hpp"

#include "registry.hpp"
#include "context.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>


namespace
{


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


} // namespace


/*
    ================================================================
    GLOBAL PREFIX
    ================================================================
*/

bool prepareGlobalPrefix(
    Context& ctx
)
{
    const auto home =
        getHome();


    if(home.empty())
    {
        std::cerr
            << "Could not determine HOME."
            << std::endl;

        return false;
    }


    ctx.globalPrefixDirectory =
        home /
        ".RetroDisc";


    std::error_code ec;

    std::filesystem::create_directories(
        ctx.globalPrefixDirectory,
        ec
    );


    if(ec)
    {
        std::cerr
            << "Could not create global RetroDisc directory:"
            << std::endl
            << "    "
            << ctx.globalPrefixDirectory
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
    LOCAL PREFIX
    ================================================================
*/

bool prepareLocalPrefix(
    Context& ctx
)
{
    const auto home =
        getHome();


    if(home.empty())
    {
        std::cerr
            << "Could not determine HOME."
            << std::endl;

        return false;
    }


    std::filesystem::path localGameDirectory;


    if(!ctx.dataPath.empty())
    {
        localGameDirectory =
            ctx.dataPath;
    }
    else
    {
        localGameDirectory =
            home /
            "Games" /
            "RetroDisc" /
            ctx.gameId;
    }


    const auto localPrefix =
        localGameDirectory /
        "pfx";


    ctx.prefixOverlayDirectory =
        localPrefix;


    try
    {
        std::filesystem::create_directories(
            localGameDirectory
        );
    }
    catch(const std::exception& e)
    {
        std::cerr
            << "Could not create local game directory:"
            << std::endl
            << "    "
            << localGameDirectory
            << std::endl
            << e.what()
            << std::endl;

        return false;
    }


    /*
        ============================================================
        EXISTING PREFIX
        ============================================================
    */

    if(prefixLooksValid(
        localPrefix
    ))
    {
        std::cout
            << "Persistent game Wine prefix:"
            << std::endl
            << "    "
            << localPrefix
            << std::endl;

        return true;
    }


    /*
        ============================================================
        EXISTING BUT INVALID PREFIX
        ============================================================
    */

    if(directoryExists(
        localPrefix
    ))
    {
        std::cerr
            << "Persistent game prefix exists but is incomplete:"
            << std::endl
            << "    "
            << localPrefix
            << std::endl;

        std::cerr
            << "Expected:"
            << std::endl
            << "    drive_c/"
            << std::endl
            << "    dosdevices/"
            << std::endl
            << "    system.reg"
            << std::endl
            << "    user.reg"
            << std::endl;

        return false;
    }


    /*
        ============================================================
        BUNDLED PREFIX
        ============================================================
    */

    const auto bundledPrefix =
        ctx.root /
        "pfx";


    if(prefixLooksValid(
        bundledPrefix
    ))
    {
        std::cout
            << "No persistent game prefix found."
            << std::endl;


        std::cout
            << "Bundled prefix found:"
            << std::endl
            << "    "
            << bundledPrefix
            << std::endl;


        if(!copyWinePrefix(
            bundledPrefix,
            localPrefix
        ))
        {
            std::cerr
                << "Could not copy bundled Wine prefix."
                << std::endl;

            return false;
        }


        return true;
    }


    /*
        ============================================================
        NO PREFIX
        ============================================================
    */

    std::cerr
        << "No Wine prefix found."
        << std::endl
        << "Persistent:"
        << std::endl
        << "    "
        << localPrefix
        << std::endl
        << "Bundled:"
        << std::endl
        << "    "
        << bundledPrefix
        << std::endl;


    return false;
}


/*
    ================================================================
    PREPARE PREFIX
    ================================================================
*/

bool preparePrefix(
    Context& ctx
)
{
    if(!prepareGlobalPrefix(
        ctx
    ))
    {
        return false;
    }


    if(!prepareLocalPrefix(
        ctx
    ))
    {
        return false;
    }


    ctx.prefixOverlayMounted =
        false;


    std::cout
        << std::endl
        << "================================================"
        << std::endl
        << "WINE PREFIX"
        << std::endl
        << "================================================"
        << std::endl;


    std::cout
        << "Prefix:"
        << std::endl
        << "    "
        << ctx.prefixOverlayDirectory
        << std::endl;


    std::cout
        << "Prefix overlay: disabled."
        << std::endl;


    return true;
}