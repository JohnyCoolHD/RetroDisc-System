#include "prefix_internal.hpp"
#include "../registry_internal.hpp"
#include "context.hpp"
#include "../prefix_sanitize.hpp"
#include "../../filesystem/filesystem_internal.hpp"

#include <filesystem>
#include <iostream>
#include <string>
#include <system_error>
#include <unistd.h>


namespace prefix_internal
{


/*
    ================================================================
    PREPARE GAME PREFIX
    ================================================================
*/

bool prepareGamePrefix(Context& ctx)
{
    if(!initializeGlobalPrefix(ctx))
    {
        return false;
    }


    if(ctx.gameDirectory.empty())
    {
        std::cerr
            << "Game directory is empty."
            << std::endl;

        return false;
    }


    /*
        Persistent game data:

            <datapath>/
            └── prefix/
                └── pfx/
                    └── only persistent changes

        The complete Wine/Proton prefix lives in:

            ~/.RetroDisc/prefix/wine/<version>/pfx
            ~/.RetroDisc/prefix/proton/<version>/pfx

        and is used as the overlay lower directory.
    */

    const auto persistentGameDirectory =
        ctx.dataPath.empty()
            ? getHome() /
              "Games" /
              "RetroDisc" /
              ctx.gameId
            : ctx.dataPath;


    const auto temporaryPrefixDirectory =
        std::filesystem::path("/tmp") /
        (
            ctx.gameId +
            "-" +
            std::to_string(getpid())
        );


    ctx.prefixLowerDirectory =
        ctx.globalPrefixDirectory;


    ctx.prefixOverlayDirectory =
        persistentGameDirectory /
        "prefix";


    ctx.prefixRuntimeUpperDirectory =
        temporaryPrefixDirectory /
        "prefix_upper";


    ctx.prefixWorkDirectory =
        temporaryPrefixDirectory /
        "prefix_work";


    /*
        The persistent prefix upper must itself remain
        a delta. Never copy the complete global prefix
        into this directory.
    */

    if(!sanitizePersistentPrefixDirectory(
        ctx.prefixOverlayDirectory,
        ctx.prefixLowerDirectory
    ))
    {  
        return false;
    }


    ctx.prefixMergedDirectory =
        temporaryPrefixDirectory /
        "merged_prefix";


    ctx.prefixMergedPfxDirectory =
        ctx.prefixMergedDirectory /
        "pfx";


    std::error_code ec;


    std::filesystem::create_directories(
        temporaryPrefixDirectory,
        ec
    );


    if(ec)
    {
        std::cerr
            << "Could not create temporary prefix directory:"
            << std::endl
            << "    "
            << temporaryPrefixDirectory
            << std::endl
            << "    "
            << ec.message()
            << std::endl;

        return false;
    }


    ec.clear();

    std::filesystem::create_directories(
        ctx.prefixRuntimeUpperDirectory,
        ec
    );

    if(ec)
    {
        std::cerr
            << "Could not create temporary prefix runtime upper:"
            << std::endl
            << "    "
            << ctx.prefixRuntimeUpperDirectory
            << std::endl
            << "    "
            << ec.message()
            << std::endl;

        return false;
    }


    /*
        Check whether the persistent upper already exists.

        IMPORTANT:

        We do NOT create a complete prefix here.

        If it does not exist, we only create the upper
        directory itself. The complete pfx/ comes from
        the lower directory through fuse-overlayfs.
    */

    ec.clear();

    const bool persistentPrefixExists =
        std::filesystem::exists(
            ctx.prefixOverlayDirectory,
            ec
        );


    if(ec)
    {
        std::cerr
            << "Could not inspect persistent prefix upper:"
            << std::endl
            << "    "
            << ctx.prefixOverlayDirectory
            << std::endl
            << "    "
            << ec.message()
            << std::endl;

        return false;
    }


    const bool useWine =
        ctx.runtime != "proton";


    if(persistentPrefixExists)
    {
        std::cout
            << "Persistent game prefix upper found:"
            << std::endl
            << "    "
            << ctx.prefixOverlayDirectory
            << std::endl;


        /*
            Remove files from the persistent upper which
            are byte-identical to the lower.

            This keeps the game prefix a real delta.
        */

        if(!compactPersistentPrefix(
            ctx.prefixOverlayDirectory,
            ctx.prefixLowerDirectory
        ))
        {
            std::cerr
                << "Could not compact persistent prefix upper."
                << std::endl;

            return false;
        }
    }
    else
    {
        /*
            IMPORTANT:

            Do NOT copy the global prefix here.

            Do NOT call copyCompletePrefix().

            The persistent prefix starts empty and fuse-overlayfs
            exposes the complete global prefix through lowerdir.
        */

        std::filesystem::create_directories(
            ctx.prefixOverlayDirectory,
            ec
        );


        if(ec)
        {
            std::cerr
                << "Could not create persistent prefix upper:"
                << std::endl
                << "    "
                << ctx.prefixOverlayDirectory
                << std::endl
                << "    "
                << ec.message()
                << std::endl;

            return false;
        }


        if(useWine)
        {
            /*
                A bundled Wine prefix is a special case.

                The bundled prefix is a direct Wine prefix:

                    bundled/pfx/
                        drive_c/
                        dosdevices/
                        system.reg
                        user.reg

                We create only the differences against
                the global Wine prefix in the persistent
                upper.
            */

            const auto executablePath =
                std::filesystem::path(
                    ctx.executable
                );


            const auto executableParent =
                executablePath.parent_path();


            const auto bundledPrefix =
                executableParent.empty()
                    ? ctx.root / "pfx"
                    : ctx.root /
                      executableParent /
                      "pfx";


            std::filesystem::path bundledPrefixToCopy =
                bundledPrefix;


            if(
                !std::filesystem::is_directory(
                    bundledPrefixToCopy
                ) &&
                bundledPrefixToCopy !=
                    ctx.root / "pfx" &&
                std::filesystem::is_directory(
                    ctx.root / "pfx"
                )
            )
            {
                bundledPrefixToCopy =
                    ctx.root / "pfx";
            }


            if(std::filesystem::is_directory(
                bundledPrefixToCopy
            ))
            {
                std::cout
                    << "Bundled game Wine prefix found:"
                    << std::endl
                    << "    "
                    << bundledPrefixToCopy
                    << std::endl;


                /*
                    IMPORTANT:

                    copyBundledPrefix() creates a DELTA
                    against the global prefix.

                    It must NOT create a complete copy.
                */

                if(!copyBundledPrefix(
                    bundledPrefixToCopy,
                    ctx.prefixOverlayDirectory / "pfx",
                    ctx.globalPrefixDirectory
                ))
                {
                    std::cerr
                        << "Could not create bundled Wine prefix delta."
                        << std::endl;

                    return false;
                }
            }
            else
            {
                /*
                    No bundled prefix.

                    Only create the upper root.

                    We intentionally do NOT create/copy
                    prefix/pfx here. The lower prefix provides
                    that through the overlay.
                */

                std::cout
                    << "No bundled Wine prefix found."
                    << std::endl
                    << "Creating empty persistent Wine prefix upper."
                    << std::endl;
            }
        }
        else
        {
            std::cout
                << "Creating empty persistent Proton prefix upper."
                << std::endl;
        }
    }


    /*
        Wine needs the compatibility symlink inside the
        persistent upper.

        Do this AFTER compaction so the intentional symlink
        is not removed by the delta cleanup.
    */

    if(useWine)
    {
        const auto persistentPfx =
            ctx.prefixOverlayDirectory /
            "pfx";


        /*
            For an empty upper this creates:

                prefix/pfx/drive_c/users/RetroDisc

            without copying the global prefix.
        */

        if(!ensureWineUserSymlink(
            persistentPfx
        ))
        {
            return false;
        }
    }


    /*
        Do not copy or clone the global prefix here.

        The only thing that must exist before mounting is:

            <datapath>/prefix/

        plus, where required, actual persistent changes
        such as bundled-prefix differences or the Wine
        RetroDisc user symlink.
    */

    if(!sanitizePersistentPrefixDirectory(
        ctx.prefixOverlayDirectory,
        ctx.prefixLowerDirectory
    ))
    {
        return false;
    }

    /*
        IMPORTANT:

        Do NOT replicate the lower prefix directory structure into the
        persistent upper.

        fuse-overlayfs already exposes lower-only directories through
        the merged view. Creating tens of thousands of empty upper
        directories defeats the delta-prefix design, wastes hundreds
        of MiB in filesystem metadata, and makes every startup slow.

        The upper contains only actual persistent changes (files,
        symlinks, whiteouts, and directories required by those changes).
    */

    std::cout
        << "Persistent prefix upper kept as a sparse delta."
        << std::endl;

    std::cout
        << "Mounting prefix overlay..."
        << std::endl;


    if(!mountPrefixOverlay(ctx))
    {
        return false;
    }


    if(
        ctx.prefixMergedPfxDirectory.empty() ||
        !std::filesystem::exists(
            ctx.prefixMergedPfxDirectory
        )
    )
    {
        std::cerr
            << "Merged prefix directory does not exist:"
            << std::endl
            << "    "
            << ctx.prefixMergedPfxDirectory
            << std::endl;


        unmountPath(
            ctx.prefixMergedDirectory
        );


        ctx.prefixOverlayMounted =
            false;


        return false;
    }


    std::cout
        << "Wine/Proton merged prefix ready:"
        << std::endl
        << "    "
        << ctx.prefixMergedPfxDirectory
        << std::endl;


    return true;
}


} // namespace prefix_internal
