#include "filesystem_internal.hpp"

#include "filesystem.hpp"

#include <filesystem>
#include <iostream>


/*
    ================================================================
    MOUNT GAME OVERLAY
    ================================================================
*/

bool mountOverlay(
    Context& ctx
)
{
    if(ctx.overlayMounted)
    {
        return true;
    }


    if(
        ctx.gameDirectory.empty() ||
        ctx.gameOverlayUpperDirectory.empty() ||
        ctx.mergedDirectory.empty() ||
        ctx.overlayWorkDirectory.empty()
    )
    {
        std::cerr
            << "Game overlay paths are incomplete."
            << std::endl;

        return false;
    }


    std::error_code ec;


    const auto gameStatus =
        std::filesystem::symlink_status(
            ctx.gameDirectory,
            ec
        );


    if(
        ec ||
        !std::filesystem::is_directory(
            gameStatus
        )
    )
    {
        std::cerr
            << "GameData is not a directory:"
            << std::endl
            << "    "
            << ctx.gameDirectory
            << std::endl;

        return false;
    }


    if(!replicateDirectoryStructure(
        ctx.gameDirectory,
        ctx.gameOverlayUpperDirectory
    ))
    {
        return false;
    }


    if(isMounted(
        ctx.mergedDirectory
    ))
    {
        std::cerr
            << "Game overlay is already mounted:"
            << std::endl
            << "    "
            << ctx.mergedDirectory
            << std::endl;

        return false;
    }


    /*
        ============================================================
        FUSE OVERLAY
        ============================================================
    */

    const std::string command =
        "fuse-overlayfs "
        "-o lowerdir=" +
        shellQuote(
            ctx.gameDirectory.string()
        ) +
        " "
        "-o upperdir=" +
        shellQuote(
            ctx.gameOverlayUpperDirectory.string()
        ) +
        " "
        "-o workdir=" +
        shellQuote(
            ctx.overlayWorkDirectory.string()
        ) +
        " " +
        shellQuote(
            ctx.mergedDirectory.string()
        );


    std::cout
        << "Mounting game overlay..."
        << std::endl;


    if(!runCommand(
        command
    ))
    {
        std::cerr
            << "Game overlay mount failed."
            << std::endl;

        return false;
    }


    ctx.overlayMounted =
        true;


    /*
        ============================================================
        EXECUTABLE VALIDATION
        ============================================================
    */

    const auto executable =
        ctx.mergedDirectory /
        ctx.executable;


    std::error_code executableError;

    const auto executableStatus =
        std::filesystem::symlink_status(
            executable,
            executableError
        );


    if(
        executableError ||
        !(
            std::filesystem::is_regular_file(
                executableStatus
            ) ||
            std::filesystem::is_symlink(
                executableStatus
            )
        )
    )
    {
        std::cerr
            << "Executable is not visible through game overlay:"
            << std::endl
            << "    "
            << executable
            << std::endl;


        unmountPath(
            ctx.mergedDirectory
        );


        ctx.overlayMounted =
            false;


        return false;
    }


    std::cout
        << "Overlay executable: "
        << executable
        << std::endl;


    return true;
}


/*
    ================================================================
    PREFIX OVERLAY
    ================================================================

    Kept only for API compatibility.

    The current architecture does NOT use a prefix overlay.
*/

bool mountPrefixOverlay(
    Context&
)
{
    std::cerr
        << "Prefix overlay is disabled."
        << std::endl;

    return false;
}