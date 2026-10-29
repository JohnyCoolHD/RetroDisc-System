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

    /*
        Bundled prefix layout inside a release:

            <root>/
            └── prefix/
                └── pfx/

        If <datapath> is the release root itself, the persistent
        prefix directory IS the bundled prefix directory. In that
        case it must be used as is: no copy, no sanitize, no compact.
    */

    const auto bundledPrefixDirectory =
        ctx.root /
        "prefix";


    std::error_code equivalentError;

    bool bundledIsPersistent =
        std::filesystem::equivalent(
            bundledPrefixDirectory,
            ctx.prefixOverlayDirectory,
            equivalentError
        );

    if(equivalentError)
    {
        bundledIsPersistent = false;
    }


    if(
        !bundledIsPersistent &&
        !sanitizePersistentPrefixDirectory(
            ctx.prefixOverlayDirectory,
            ctx.prefixLowerDirectory
        )
    )
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


    /*
        An empty persistent prefix (for example left over from an
        earlier launch without bundled prefix support) counts as
        not existing, so the bundled prefix can still be copied.
    */

    bool persistentPrefixUsable =
        persistentPrefixExists;

    if(persistentPrefixUsable)
    {
        std::error_code emptyError;

        if(
            std::filesystem::is_directory(
                ctx.prefixOverlayDirectory,
                emptyError
            ) &&
            std::filesystem::is_empty(
                ctx.prefixOverlayDirectory,
                emptyError
            ) &&
            !emptyError
        )
        {
            persistentPrefixUsable = false;
        }
    }


    const bool useWine =
        ctx.runtime != "proton";


    if(persistentPrefixUsable)
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

        if(
            !bundledIsPersistent &&
            !compactPersistentPrefix(
                ctx.prefixOverlayDirectory,
                ctx.prefixLowerDirectory
            )
        )
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


        /*
            A bundled prefix (Wine and Proton) lives in:

                <root>/prefix/pfx/
                    drive_c/
                    dosdevices/
                    system.reg
                    user.reg

            Only the differences against the global prefix are
            created in the persistent upper.
        */

        const auto bundledPrefixToCopy =
            bundledPrefixDirectory /
            "pfx";


        if(std::filesystem::is_directory(
            bundledPrefixToCopy
        ))
        {
            std::cout
                << "Bundled game prefix found:"
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
                    << "Could not create bundled prefix delta."
                    << std::endl;

                return false;
            }
        }
        else
        {
            std::cout
                << "No bundled prefix found."
                << std::endl
                << "Creating empty persistent prefix upper."
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

    if(
        !bundledIsPersistent &&
        !sanitizePersistentPrefixDirectory(
            ctx.prefixOverlayDirectory,
            ctx.prefixLowerDirectory
        )
    )
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
