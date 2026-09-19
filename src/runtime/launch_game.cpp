#include "runtime_internal.hpp"
#include "runtime.hpp"
#include "filesystem.hpp"
#include "context.hpp"

#include <filesystem>
#include <iostream>
#include <string>
#include <system_error>
#include <sys/wait.h>


using namespace runtime_internal;


/*
    =================================================================
    PUBLIC LAUNCH FUNCTION
    =================================================================
*/

bool launchGame(
    Context& ctx
)
{
    if(!ctx.overlayMounted)
    {
        std::cerr
            << "Game overlay is not mounted."
            << std::endl;

        return false;
    }

    if(ctx.prefixMergedPfxDirectory.empty())
    {
        std::cerr
            << "Wine prefix path is empty."
            << std::endl;

        cleanupFilesystem(ctx);

        return false;
    }


    /*
        ============================================================
        GAME
        ============================================================
    */

    const auto gameDirectory =
        ctx.mergedDirectory;

    const auto executable =
        gameDirectory /
        ctx.executable;

    std::error_code ec;

    const auto gameStatus =
        std::filesystem::symlink_status(
            gameDirectory,
            ec
        );

    if(
        ec ||
        !std::filesystem::is_directory(gameStatus)
    )
    {
        std::cerr
            << "Merged game directory does not exist:"
            << std::endl
            << "    "
            << gameDirectory
            << std::endl;

        cleanupFilesystem(ctx);

        return false;
    }

    const auto executableStatus =
        std::filesystem::symlink_status(
            executable,
            ec
        );

    if(
        ec ||
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
            << "Executable not found:"
            << std::endl
            << "    "
            << executable
            << std::endl;

        cleanupFilesystem(ctx);

        return false;
    }


    /*
        ============================================================
        PREFIX
        ============================================================
    */

    const auto prefixDirectory =
        ctx.prefixMergedPfxDirectory;

    const auto prefixStatus =
        std::filesystem::symlink_status(
            prefixDirectory,
            ec
        );

    if(
        ec ||
        !std::filesystem::is_directory(prefixStatus)
    )
    {
        std::cerr
            << "Wine/Proton merged prefix not found:"
            << std::endl
            << "    "
            << prefixDirectory
            << std::endl;

        cleanupFilesystem(ctx);

        return false;
    }


    /*
        ============================================================
        RUNTIME USER
        ============================================================
    */

    const std::string runtimeUser =
        determineRuntimeUser(ctx);

    if(runtimeUser != CANONICAL_WINDOWS_USER)
    {
        std::cerr
            << "Invalid RetroDisc runtime user."
            << std::endl;

        cleanupFilesystem(ctx);

        return false;
    }

    /*
        This is deliberately performed for BOTH Wine and Proton.

        The canonical profile is always:

            drive_c/users/RetroDisc

        Broken legacy/canonical symlinks are repaired before the
        runtime is started.
    */

    if(!prepareRuntimeUser(
        ctx,
        runtimeUser
    ))
    {
        cleanupFilesystem(ctx);

        return false;
    }


    /*
        ============================================================
        CONFIGURATION
        ============================================================
    */

    std::cout
        << std::endl
        << "================================================"
        << std::endl
        << "WINE / PROTON CONFIGURATION"
        << std::endl
        << "================================================"
        << std::endl;

    std::cout
        << "Merged prefix:"
        << std::endl
        << "    "
        << prefixDirectory
        << std::endl;

    std::cout
        << "Runtime:"
        << std::endl
        << "    "
        << ctx.runtime
        << std::endl;

    std::cout
        << "Windows user:"
        << std::endl
        << "    "
        << CANONICAL_WINDOWS_USER
        << std::endl;

    std::cout
        << "Canonical Windows profile:"
        << std::endl
        << "    "
        << (
            prefixDirectory /
            "drive_c" /
            "users" /
            CANONICAL_WINDOWS_USER
        )
        << std::endl;

    std::cout
        << "Windows version:"
        << std::endl
        << "    "
        << (
            ctx.wine.windowsVersion.empty()
                ? "unchanged"
                : ctx.wine.windowsVersion
        )
        << std::endl;

    std::cout
        << "Renderer:"
        << std::endl
        << "    "
        << (
            ctx.wine.graphics.renderer.empty()
                ? "auto"
                : ctx.wine.graphics.renderer
        )
        << std::endl;

    std::cout
        << "Video memory:"
        << std::endl
        << "    "
        << ctx.wine.graphics.videoMemory
        << " MB"
        << std::endl;

    std::cout
        << "Strict draw ordering:"
        << std::endl
        << "    "
        << (
            ctx.wine.graphics.strictDrawOrdering
                ? "enabled"
                : "disabled"
        )
        << std::endl;

    std::cout
        << "Virtual desktop:"
        << std::endl
        << "    "
        << (
            ctx.wine.display.virtualDesktop.enabled
                ? "enabled"
                : "disabled"
        )
        << std::endl;

    if(ctx.wine.display.virtualDesktop.enabled)
    {
        std::cout
            << "Virtual desktop size:"
            << std::endl
            << "    "
            << ctx.wine.display.virtualDesktop.width
            << "x"
            << ctx.wine.display.virtualDesktop.height
            << std::endl;
    }

    std::cout
        << "Window managed:"
        << std::endl
        << "    "
        << (
            ctx.wine.display.window.managed
                ? "yes"
                : "no"
        )
        << std::endl;

    std::cout
        << "Window decorations:"
        << std::endl
        << "    "
        << (
            ctx.wine.display.window.decorations
                ? "yes"
                : "no"
        )
        << std::endl;

    std::cout
        << "Mouse capture:"
        << std::endl
        << "    "
        << (
            ctx.wine.display.window.mouseCapture
                ? "yes"
                : "no"
        )
        << std::endl;

    std::cout
        << "DPI:"
        << std::endl
        << "    "
        << ctx.wine.display.dpi
        << std::endl;

    std::cout
        << "ESYNC:"
        << std::endl
        << "    "
        << (
            ctx.wine.sync.esync
                ? "enabled"
                : "disabled"
        )
        << std::endl;

    std::cout
        << "FSYNC:"
        << std::endl
        << "    "
        << (
            ctx.wine.sync.fsync
                ? "enabled"
                : "disabled"
        )
        << std::endl;

    std::cout
        << "NTSYNC:"
        << std::endl
        << "    "
        << (
            ctx.wine.sync.ntsync
                ? "enabled"
                : "disabled"
        )
        << std::endl;


    /*
        ============================================================
        LAUNCH
        ============================================================
    */

    std::cout
        << std::endl
        << "Launching "
        << ctx.runtime
        << "..."
        << std::endl;

    std::cout
        << "Merged Wine/Proton prefix:"
        << std::endl
        << "    "
        << prefixDirectory
        << std::endl;

    std::cout
        << "Runtime user:"
        << std::endl
        << "    "
        << CANONICAL_WINDOWS_USER
        << std::endl;

    std::cout
        << "Persistent user:"
        << std::endl
        << "    "
        << CANONICAL_WINDOWS_USER
        << std::endl;

    std::cout
        << "Linux user is NOT used as Windows user."
        << std::endl;

    printEnvironment(ctx);


    /*
        ============================================================
        BUILD COMMAND
        ============================================================
    */

    std::string command;

    if(ctx.runtime == "proton")
    {
        command =
            buildProtonCommand(
                ctx,
                runtimeUser
            );
    }
    else
    {
        command =
            buildWineCommand(
                ctx,
                runtimeUser
            );
    }

    if(command.empty())
    {
        cleanupRuntimeUser(
            ctx,
            runtimeUser
        );

        cleanupFilesystem(ctx);

        return false;
    }


    /*
        ============================================================
        EXECUTE
        ============================================================
    */

    const int result =
        runCommand(command);


    /*
        ============================================================
        CLEANUP
        ============================================================
    */

    cleanupRuntimeUser(
        ctx,
        runtimeUser
    );

    const bool cleanupResult =
        cleanupFilesystem(ctx);

    if(result == -1)
    {
        return false;
    }

    if(!WIFEXITED(result))
    {
        return false;
    }

    const int exitCode =
        WEXITSTATUS(result);

    if(!cleanupResult)
    {
        return false;
    }

    if(exitCode != 0)
    {
        std::cout
            << "Game exited with code "
            << exitCode
            << "."
            << std::endl;

        return false;
    }

    return true;
}
