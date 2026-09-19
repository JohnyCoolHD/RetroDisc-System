#include "prefix_sanitize_internal.hpp"
#include "../prefix_sanitize.hpp"

#include <array>
#include <filesystem>
#include <iostream>
#include <system_error>


using namespace prefix_sanitize_internal;


bool sanitizePersistentPrefixDirectory(
    const std::filesystem::path& prefixDirectory,
    const std::filesystem::path& lowerPrefixDirectory
)
{
    std::error_code ec;

    const auto status =
        std::filesystem::symlink_status(
            prefixDirectory,
            ec
        );

    if(
        ec == std::errc::no_such_file_or_directory ||
        status.type() == std::filesystem::file_type::not_found
    )
    {
        return true;
    }

    if(
        ec ||
        !std::filesystem::is_directory(status)
    )
    {
        std::cerr
            << "Persistent prefix path is not a directory:"
            << std::endl
            << "    "
            << prefixDirectory
            << std::endl;

        if(ec)
        {
            std::cerr
                << "    "
                << ec.message()
                << std::endl;
        }

        return false;
    }


    /*
        These files belong to Proton's compat-data bookkeeping.

        They must never become persistent game data.
    */

    static constexpr std::array<const char*, 4> metadataFiles =
    {
        "version",
        "tracked_files",
        "config_info",
        "pfx.lock"
    };


    for(const auto* filename : metadataFiles)
    {
        if(!removeOptionalEntry(
            prefixDirectory / filename,
            "metadata"
        ))
        {
            return false;
        }
    }


    /*
        The Windows directory itself is NOT removed.

        Instead, only regular files which are byte-identical
        to the corresponding file in the runtime-specific
        lower prefix are removed.

        This keeps the persistent prefix a delta while allowing
        custom Windows files to remain in the game upper.
    */

    const auto upperWindowsDirectory =
        prefixDirectory /
        "pfx" /
        "drive_c" /
        "windows";

    const auto lowerWindowsDirectory =
        lowerPrefixDirectory /
        "pfx" /
        "drive_c" /
        "windows";


    if(!removeRedundantWindowsFiles(
        upperWindowsDirectory,
        lowerWindowsDirectory
    ))
    {
        return false;
    }


    /*
        Never remove:

            pfx/system.reg
            pfx/user.reg
            pfx/userdef.reg
            pfx/drive_c/users/RetroDisc

        They are persistent Wine prefix/game state.
    */

    return true;
}
