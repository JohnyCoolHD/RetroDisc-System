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
        NEVER OVERWRITE AN EXISTING CONFIG
        ============================================================
    */

    std::error_code ec;

    if(std::filesystem::exists(configPath, ec))
    {
        if(ec)
        {
            std::cerr
                << "Could not inspect existing game config:"
                << std::endl
                << "    "
                << configPath
                << std::endl
                << "    "
                << ec.message()
                << std::endl;

            return false;
        }

        std::cout
            << "Game config already exists:"
            << std::endl
            << "    "
            << configPath
            << std::endl;

        return true;
    }


    /*
        ============================================================
        CREATE GAME DIRECTORY
        ============================================================
    */

    ec.clear();

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
        OPTIONAL EXTERNAL CONFIG
        ============================================================

        An external config.json may be placed next to the game
        executable:

            <root>/config.json

        If it does not exist, this is NOT an error.

        In that case the embedded runtime configuration is used.
    */

    const auto externalConfigPath =
        ctx.root /
        "config.json";


    ec.clear();

    const bool externalConfigExists =
        std::filesystem::exists(
            externalConfigPath,
            ec
        );

    if(ec)
    {
        std::cerr
            << "Could not inspect config next to executable:"
            << std::endl
            << "    "
            << externalConfigPath
            << std::endl
            << "    "
            << ec.message()
            << std::endl;

        return false;
    }


    /*
        ============================================================
        COPY EXTERNAL CONFIG
        ============================================================
    */

    if(externalConfigExists)
    {
        ec.clear();

        const bool regularFile =
            std::filesystem::is_regular_file(
                externalConfigPath,
                ec
            );

        if(ec)
        {
            std::cerr
                << "Could not inspect external config:"
                << std::endl
                << "    "
                << externalConfigPath
                << std::endl
                << "    "
                << ec.message()
                << std::endl;

            return false;
        }

        if(!regularFile)
        {
            std::cerr
                << "External config is not a regular file:"
                << std::endl
                << "    "
                << externalConfigPath
                << std::endl;

            return false;
        }


        ec.clear();

        if(!std::filesystem::copy_file(
            externalConfigPath,
            configPath,
            std::filesystem::copy_options::none,
            ec
        ))
        {
            std::cerr
                << "Could not copy external game config:"
                << std::endl
                << "    "
                << externalConfigPath
                << std::endl
                << "to:"
                << std::endl
                << "    "
                << configPath
                << std::endl
                << "    "
                << ec.message()
                << std::endl;

            return false;
        }


        std::cout
            << "Copied game config:"
            << std::endl
            << "    "
            << externalConfigPath
            << std::endl
            << "→"
            << std::endl
            << "    "
            << configPath
            << std::endl;

        return true;
    }


    /*
        ============================================================
        NO EXTERNAL CONFIG
        ============================================================

        Use the configuration compiled into RetroDisc.

        runtime_config.cpp provides the embedded:
            runtime/proton.json
            runtime/wine.json
    */

    std::string runtimeConfig;


    if(!getEmbeddedRuntimeConfig(
        ctx.runtime,
        runtimeConfig
    ))
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
        WRITE EMBEDDED CONFIG
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


    file.close();


    if(file.fail())
    {
        std::cerr
            << "Could not finalize config:"
            << std::endl
            << "    "
            << configPath
            << std::endl;

        return false;
    }


    std::cout
        << "Created game config from embedded runtime:"
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
