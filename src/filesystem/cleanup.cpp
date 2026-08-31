#include "filesystem_internal.hpp"

#include "filesystem.hpp"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>


/*
    ================================================================
    CLEANUP FILESYSTEM
    ================================================================
*/

bool cleanupFilesystem(
    Context& ctx
)
{
    bool success =
        true;


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
                std::chrono::milliseconds(
                    250
                )
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

            success =
                false;
        }
        else
        {
            ctx.overlayMounted =
                false;
        }
    }


    /*
        ============================================================
        TEMPORARY DIRECTORIES
        ============================================================
    */

    if(!ctx.overlayMounted)
    {
        if(!removeDirectory(
            ctx.mergedDirectory
        ))
        {
            success =
                false;
        }


        if(!removeDirectory(
            ctx.overlayWorkDirectory.parent_path()
        ))
        {
            success =
                false;
        }
    }


    /*
        ============================================================
        PREFIX
        ============================================================
    */

    ctx.prefixOverlayMounted =
        false;


    return success;
}


/*
    ================================================================
    GENERAL CLEANUP
    ================================================================
*/

bool cleanup(
    Context& ctx
)
{
    return cleanupFilesystem(ctx);
}