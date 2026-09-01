#include "filesystem_internal.hpp"

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

        Proton may return before wineserver has finished writing
        the registry.

        The prefix overlay must therefore remain mounted until
        wineserver has completely shut down.
    */


    if(ctx.prefixOverlayMounted)
    {
        const auto winePrefix =
            ctx.prefixMergedDirectory;


        if(
            !winePrefix.empty() &&
            std::filesystem::exists(winePrefix)
        )
        {
            std::cout
                << "Waiting for Wine server to finish..."
                << std::endl;


            const std::string command =
                "WINEPREFIX=" +
                shellQuote(
                    winePrefix.string()
                ) +
                " wineserver -w";


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
        WINE PREFIX OVERLAY
        ============================================================
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
            ctx.overlayWorkDirectory
        );
    }


    /*
        ============================================================
        TEMPORARY PREFIX DIRECTORIES
        ============================================================
    */


    if(!ctx.prefixOverlayMounted)
    {
        removeDirectory(
            ctx.prefixMergedDirectory
        );


        /*
            IMPORTANT:

            Only remove .prefix_work itself.

            Never remove its parent directory because that parent
            contains the persistent per-game pfx.
        */


        removeDirectory(
            ctx.prefixWorkDirectory
        );
    }


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