#include "filesystem_internal.hpp"
#include "../registry/prefix_sanitize.hpp"
#include "filesystem.hpp"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>


bool cleanupFilesystem(Context& ctx)
{
    bool success = true;

    /*
        ============================================================
        WAIT FOR WINE / PROTON
        ============================================================

        The prefix overlay must remain mounted until wineserver
        has completely shut down.
    */

    if(ctx.prefixOverlayMounted)
    {
        const auto winePrefix =
            ctx.prefixMergedPfxDirectory;

        if(
            !winePrefix.empty() &&
            std::filesystem::exists(winePrefix)
        )
        {
            std::cout
                << "Waiting for Wine server to finish..."
                << std::endl;

            std::string command;

            if(
                ctx.runtime == "proton" &&
                !ctx.resolvedProtonPath.empty()
            )
            {
                const auto protonWineServer =
                    ctx.resolvedProtonPath.parent_path() /
                    "files" /
                    "bin" /
                    "wineserver";

                std::error_code protonServerError;

                if(std::filesystem::is_regular_file(
                    protonWineServer,
                    protonServerError
                ))
                {
                    command =
                        "WINEPREFIX=" +
                        shellQuote(winePrefix.string()) +
                        " " +
                        shellQuote(protonWineServer.string()) +
                        " -w";
                }
            }

            if(command.empty())
            {
                command =
                    "WINEPREFIX=" +
                    shellQuote(winePrefix.string()) +
                    " wineserver -w";
            }

            if(!runCommand(command, true))
            {
                std::cerr
                    << "Warning: wineserver did not exit cleanly."
                    << std::endl;
            }
            else
            {
                std::cout
                    << "Wine server finished."
                    << std::endl;
            }
        }
    }


    /*
        ============================================================
        WINE PREFIX OVERLAY
        ============================================================

        IMPORTANT:
        The prefix overlay must be unmounted BEFORE removing its
        temporary parent directory.
    */

    if(ctx.prefixOverlayMounted)
    {
        std::cout
            << "Unmounting Wine prefix overlay..."
            << std::endl;

        bool unmounted =
            unmountPath(
                ctx.prefixMergedDirectory
            );

        if(!unmounted)
        {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(250)
            );

            unmounted =
                unmountPath(
                    ctx.prefixMergedDirectory
                );
        }

        if(!unmounted)
        {
            std::cerr
                << "Could not unmount Wine prefix overlay:"
                << std::endl
                << "    "
                << ctx.prefixMergedDirectory
                << std::endl;

            success = false;
        }
        else
        {
            ctx.prefixOverlayMounted = false;

            /*
                The persistent upperdir can only be sanitized after
                the prefix overlay has been unmounted.
            */

            if(!sanitizePersistentPrefixDirectory(
                ctx.prefixOverlayDirectory,
                ctx.prefixLowerDirectory
            ))
            {
                std::cerr
                    << "Failed to sanitize persistent prefix"
                    << " directory during cleanup."
                    << std::endl;

                success = false;
            }
        }
    }


    /*
        ============================================================
        GAME OVERLAY
        ============================================================
    */

    if(ctx.overlayMounted)
    {
        std::cout
            << "Unmounting game overlay..."
            << std::endl;

        bool unmounted =
            unmountPath(
                ctx.mergedDirectory
            );

        if(!unmounted)
        {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(250)
            );

            unmounted =
                unmountPath(
                    ctx.mergedDirectory
                );
        }

        if(!unmounted)
        {
            std::cerr
                << "Could not unmount game overlay:"
                << std::endl
                << "    "
                << ctx.mergedDirectory
                << std::endl;

            success = false;
        }
        else
        {
            ctx.overlayMounted = false;
        }
    }


    /*
        ============================================================
        TEMPORARY GAME DIRECTORIES
        ============================================================
    */

    if(!ctx.overlayMounted)
    {
        removeDirectory(
            ctx.mergedDirectory
        );

        removeDirectory(
            ctx.overlayWorkDirectory.parent_path()
        );
    }


    /*
        ============================================================
        TEMPORARY PREFIX DIRECTORIES
        ============================================================
    */

    if(!ctx.prefixOverlayMounted)
    {
        /*
            Remove the temporary per-launch prefix directory.

            This contains:
                merged_prefix/

            It does NOT contain the persistent prefix.
        */

        removeDirectory(
            ctx.prefixMergedDirectory.parent_path()
        );

        /*
            Remove only .prefix_work.

            Never remove its parent because the parent contains
            the persistent game prefix.
        */

        removeDirectory(
            ctx.prefixWorkDirectory
        );
    }


    /*
        ============================================================
        FINAL STATE
        ============================================================
    */

    if(ctx.overlayMounted)
        success = false;

    if(ctx.prefixOverlayMounted)
        success = false;


    return success;
}


bool cleanup(Context& ctx)
{
    return cleanupFilesystem(ctx);
}