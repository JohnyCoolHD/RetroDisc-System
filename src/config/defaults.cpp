#include "config.hpp"

#include "config_internal.hpp"
#include "runtime_config.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>


/*
    ================================================================
    CREATE DEFAULT GAME CONFIG
    ================================================================
*/

bool createDefaultGameConfig(
    const Context& ctx
)
{
    const auto configDirectory =
        getGameConfigDirectory(ctx);

    const auto configPath =
        getGameConfigPath(ctx);


    if(
        configDirectory.empty() ||
        configPath.empty()
    )
    {
        std::cerr
            << "Could not determine game config path."
            << std::endl;

        return false;
    }


    /*
        ============================================================
        GET EMBEDDED RUNTIME CONFIG
        ============================================================
    */

    std::string runtimeConfig;


    if(
        !getEmbeddedRuntimeConfig(
            ctx.runtime,
            runtimeConfig
        )
    )
    {
        std::cerr
            << "Embedded runtime configuration not found:"
            << std::endl
            << "    "
            << ctx.runtime
            << std::endl;

        return false;
    }


    /*
        ============================================================
        CREATE DIRECTORY
        ============================================================
    */

    std::error_code ec;


    std::filesystem::create_directories(
        configDirectory,
        ec
    );


    if(ec)
    {
        std::cerr
            << "Could not create game config directory:"
            << std::endl
            << "    "
            << configDirectory
            << std::endl
            << "    "
            << ec.message()
            << std::endl;

        return false;
    }


    /*
        ============================================================
        WRITE CONFIG
        ============================================================
    */

    std::ofstream file(
        configPath
    );


    if(!file)
    {
        std::cerr
            << "Could not create config:"
            << std::endl
            << "    "
            << configPath
            << std::endl;

        return false;
    }


    file
        << runtimeConfig;


    if(!file)
    {
        std::cerr
            << "Could not write config:"
            << std::endl
            << "    "
            << configPath
            << std::endl;

        return false;
    }


    std::cout
        << "Created game config:"
        << std::endl
        << "    "
        << configPath
        << std::endl;


    std::cout
        << "Runtime:"
        << std::endl
        << "    "
        << ctx.runtime
        << std::endl;


    return true;
}
