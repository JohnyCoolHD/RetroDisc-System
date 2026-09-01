#include "filesystem_internal.hpp"
#include "filesystem.hpp"

#include <filesystem>
#include <iostream>


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
            ctx.prefixOverlayDirectory,
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
            << "Prefix upper directory is not valid:"
            << std::endl
            << "    "
            << ctx.prefixOverlayDirectory
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


    const std::string command =
        "fuse-overlayfs "
        "-o lowerdir=" +
        shellQuote(
            ctx.prefixLowerDirectory.string()
        ) +
        " "
        "-o upperdir=" +
        shellQuote(
            ctx.prefixOverlayDirectory.string()
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
        << "    Lower:"
        << std::endl
        << "        "
        << ctx.prefixLowerDirectory
        << std::endl;

    std::cout
        << "    Upper:"
        << std::endl
        << "        "
        << ctx.prefixOverlayDirectory
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


    const auto driveC =
        ctx.prefixMergedDirectory /
        "drive_c";


    const auto dosDevices =
        ctx.prefixMergedDirectory /
        "dosdevices";


    const auto systemReg =
        ctx.prefixMergedDirectory /
        "system.reg";


    const auto userReg =
        ctx.prefixMergedDirectory /
        "user.reg";


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
        << ctx.prefixMergedDirectory
        << std::endl;


    return true;
}