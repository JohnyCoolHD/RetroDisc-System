#include "filesystem_internal.hpp"

#include "filesystem.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

#include <unistd.h>


/*
    ================================================================
    PREPARE FILESYSTEM
    ================================================================
*/

bool prepareFilesystem(
    Context& ctx
)
{
    if(ctx.executable.empty())
    {
        std::cerr
            << "Executable path in manifest is empty."
            << std::endl;

        return false;
    }


    /*
        ============================================================
        GAME DIRECTORY
        ============================================================
    */

    ctx.gameDirectory =
        ctx.root /
        "gamedata";


    {
        std::error_code ec;

        const auto status =
            std::filesystem::symlink_status(
                ctx.gameDirectory,
                ec
            );

        if(
            ec ||
            !std::filesystem::is_directory(status)
        )
        {
            std::cerr
                << "GameData directory does not exist:"
                << std::endl
                << "    "
                << ctx.gameDirectory
                << std::endl;

            return false;
        }
    }


    /*
        ============================================================
        EXECUTABLE
        ============================================================
    */

    std::filesystem::path executablePath =
        ctx.executable;

    std::string executable =
        executablePath
            .lexically_normal()
            .generic_string();


    const std::string prefix =
        "gamedata/";


    if(
        executable.rfind(
            prefix,
            0
        ) == 0
    )
    {
        executable =
            executable.substr(
                prefix.size()
            );
    }


    while(
        !executable.empty() &&
        executable.front() == '/'
    )
    {
        executable.erase(
            executable.begin()
        );
    }


    ctx.executable =
        executable;


    /*
        ============================================================
        HOME
        ============================================================
    */

    const char* home =
        std::getenv("HOME");

    if(home == nullptr || *home == '\0')
    {
        std::cerr
            << "HOME is not set."
            << std::endl;

        return false;
    }


    /*
        ============================================================
        PERSISTENT GAME DATA
        ============================================================
    */

    std::filesystem::path persistentGameDirectory;


    if(!ctx.dataPath.empty())
    {
        persistentGameDirectory =
            ctx.dataPath;
    }
    else
    {
        persistentGameDirectory =
            std::filesystem::path(home) /
            "Games" /
            "RetroDisc" /
            ctx.gameId;
    }


    ctx.gameOverlayUpperDirectory =
        persistentGameDirectory /
        "gamedata";


    ctx.gameDataDirectory =
        ctx.gameOverlayUpperDirectory;


    /*
        ============================================================
        TEMPORARY OVERLAY
        ============================================================
    */

    const std::string pid =
        std::to_string(
            getpid()
        );


    ctx.mergedDirectory =
        std::filesystem::path("/tmp") /
        (
            "RetroDisc_" +
            pid
        );


    ctx.overlayWorkDirectory =
        std::filesystem::path("/tmp") /
        (
            "RetroDiscWork_" +
            pid
        ) /
        "work";


    try
    {
        std::filesystem::create_directories(
            ctx.gameOverlayUpperDirectory
        );

        std::filesystem::create_directories(
            ctx.mergedDirectory
        );

        std::filesystem::create_directories(
            ctx.overlayWorkDirectory
        );
    }
    catch(const std::exception& e)
    {
        std::cerr
            << "Could not create filesystem directories:"
            << std::endl
            << e.what()
            << std::endl;

        return false;
    }


    /*
        ============================================================
        DIRECTORY STRUCTURE
        ============================================================
    */

    std::cout
        << "Replicating GameData directory structure..."
        << std::endl;


    if(!replicateDirectoryStructure(
        ctx.gameDirectory,
        ctx.gameOverlayUpperDirectory
    ))
    {
        return false;
    }


    ctx.overlayMounted =
        false;


    ctx.prefixOverlayMounted =
        false;


    /*
        ============================================================
        STATUS
        ============================================================
    */

    std::cout
        << "GameData: "
        << ctx.gameDirectory
        << std::endl;


    std::cout
        << "Game overlay upper: "
        << ctx.gameOverlayUpperDirectory
        << std::endl;


    std::cout
        << "Temporary game mount: "
        << ctx.mergedDirectory
        << std::endl;


    std::cout
        << "Executable: "
        << ctx.executable
        << std::endl;


    return mountOverlay(ctx);
}