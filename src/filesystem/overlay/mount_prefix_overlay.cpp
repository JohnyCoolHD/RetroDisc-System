#include "../filesystem_internal.hpp"
#include "filesystem.hpp"
#include "context.hpp"

#include <filesystem>
#include <iostream>
#include <string>
#include <system_error>


bool mountPrefixOverlay(
    Context& ctx
)
{
    if(ctx.prefixOverlayMounted)
    {
        return true;
    }


    if(
        ctx.prefixLowerDirectory.empty() ||
        ctx.prefixOverlayDirectory.empty() ||
        ctx.prefixRuntimeUpperDirectory.empty() ||
        ctx.prefixMergedDirectory.empty() ||
        ctx.prefixWorkDirectory.empty()
    )
    {
        std::cerr
            << "Prefix overlay paths are incomplete."
            << std::endl;

        return false;
    }


    std::error_code ec;


    const auto lowerStatus =
        std::filesystem::symlink_status(
            ctx.prefixLowerDirectory,
            ec
        );


    if(
        ec ||
        !std::filesystem::is_directory(
            lowerStatus
        )
    )
    {
        std::cerr
            << "Prefix lower directory is not valid:"
            << std::endl
            << "    "
            << ctx.prefixLowerDirectory
            << std::endl;

        return false;
    }


    ec.clear();


    const auto upperStatus =
        std::filesystem::symlink_status(
            ctx.prefixRuntimeUpperDirectory,
            ec
        );


    if(
        ec ||
        !std::filesystem::is_directory(
            upperStatus
        )
    )
    {
        std::cerr
            << "Prefix runtime upper directory is not valid:"
            << std::endl
            << "    "
            << ctx.prefixRuntimeUpperDirectory
            << std::endl;

        return false;
    }


    std::filesystem::create_directories(
        ctx.prefixMergedDirectory,
        ec
    );


    if(ec)
    {
        std::cerr
            << "Could not create prefix merged directory:"
            << std::endl
            << "    "
            << ctx.prefixMergedDirectory
            << std::endl
            << "    "
            << ec.message()
            << std::endl;

        return false;
    }


    ec.clear();


    std::filesystem::create_directories(
        ctx.prefixWorkDirectory,
        ec
    );


    if(ec)
    {
        std::cerr
            << "Could not create prefix overlay work directory:"
            << std::endl
            << "    "
            << ctx.prefixWorkDirectory
            << std::endl
            << "    "
            << ec.message()
            << std::endl;

        return false;
    }


    if(isMounted(
        ctx.prefixMergedDirectory
    ))
    {
        std::cerr
            << "Prefix overlay is already mounted:"
            << std::endl
            << "    "
            << ctx.prefixMergedDirectory
            << std::endl;

        return false;
    }


    const std::string lowerDirectories =
        ctx.prefixOverlayDirectory.string() +
        ":" +
        ctx.prefixLowerDirectory.string();


    const std::string command =
        "FUSE_OVERLAYFS_DISABLE_OVL_WHITEOUT=1 "
        "fuse-overlayfs "
        "-o lowerdir=" +
        shellQuote(
            lowerDirectories
        ) +
        " "
        "-o upperdir=" +
        shellQuote(
            ctx.prefixRuntimeUpperDirectory.string()
        ) +
        " "
        "-o workdir=" +
        shellQuote(
            ctx.prefixWorkDirectory.string()
        ) +
        " " +
        shellQuote(
            ctx.prefixMergedDirectory.string()
        );


    std::cout
        << "Mounting prefix overlay..."
        << std::endl;

    std::cout
        << "    Base lower:"
        << std::endl
        << "        "
        << ctx.prefixLowerDirectory
        << std::endl;

    std::cout
        << "    Persistent lower:"
        << std::endl
        << "        "
        << ctx.prefixOverlayDirectory
        << std::endl
        << "    Runtime upper:"
        << std::endl
        << "        "
        << ctx.prefixRuntimeUpperDirectory
        << std::endl;

    std::cout
        << "    Work:"
        << std::endl
        << "        "
        << ctx.prefixWorkDirectory
        << std::endl;

    std::cout
        << "    Merged:"
        << std::endl
        << "        "
        << ctx.prefixMergedDirectory
        << std::endl;


    if(!runCommand(
        command
    ))
    {
        std::cerr
            << "Prefix overlay mount failed."
            << std::endl;

        return false;
    }


    ctx.prefixOverlayMounted =
        true;


    /*
        Wine and Proton:

            merged_prefix/
            └── pfx/
                ├── drive_c/
                ├── dosdevices/
                ├── system.reg
                └── user.reg
    */

    const auto prefixRoot =
        ctx.prefixMergedDirectory /
        "pfx";


    const auto driveC =
        prefixRoot /
        "drive_c";

    const auto dosDevices =
        prefixRoot /
        "dosdevices";

    const auto systemReg =
        prefixRoot /
        "system.reg";

    const auto userReg =
        prefixRoot /
        "user.reg";


    /*
        Temporary diagnostics.

        These checks happen while the overlay is still mounted,
        so we can see exactly what fuse-overlayfs produced.
    */

    std::cout
        << "Prefix overlay validation:"
        << std::endl;

    std::cout
        << "    Prefix root:"
        << std::endl
        << "        "
        << prefixRoot
        << std::endl;

    std::cout
        << "    drive_c:"
        << std::endl
        << "        "
        << driveC
        << " -> "
        << (
            std::filesystem::is_directory(
                driveC
            )
            ? "OK"
            : "MISSING"
        )
        << std::endl;

    std::cout
        << "    dosdevices:"
        << std::endl
        << "        "
        << dosDevices
        << " -> "
        << (
            std::filesystem::is_directory(
                dosDevices
            )
            ? "OK"
            : "MISSING"
        )
        << std::endl;

    std::cout
        << "    system.reg:"
        << std::endl
        << "        "
        << systemReg
        << " -> "
        << (
            std::filesystem::is_regular_file(
                systemReg
            )
            ? "OK"
            : "MISSING"
        )
        << std::endl;

    std::cout
        << "    user.reg:"
        << std::endl
        << "        "
        << userReg
        << " -> "
        << (
            std::filesystem::is_regular_file(
                userReg
            )
            ? "OK"
            : "MISSING"
        )
        << std::endl;


    const bool validPrefix =
        std::filesystem::is_directory(
            driveC
        ) &&
        std::filesystem::is_directory(
            dosDevices
        ) &&
        std::filesystem::is_regular_file(
            systemReg
        ) &&
        std::filesystem::is_regular_file(
            userReg
        );


    if(!validPrefix)
    {
        std::cerr
            << "Mounted prefix overlay is invalid."
            << std::endl
            << "    "
            << ctx.prefixMergedDirectory
            << std::endl;


        unmountPath(
            ctx.prefixMergedDirectory
        );


        ctx.prefixOverlayMounted =
            false;


        return false;
    }


    std::cout
        << "Prefix overlay mounted successfully."
        << std::endl;

    std::cout
        << "Wine prefix:"
        << std::endl
        << "    "
        << prefixRoot
        << std::endl;


    return true;
}
