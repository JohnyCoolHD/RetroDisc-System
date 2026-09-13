#include "prefix_sanitize.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>


namespace
{

bool removeOptionalEntry(
    const std::filesystem::path& path,
    const char* description
)
{
    std::error_code ec;

    const auto status =
        std::filesystem::symlink_status(
            path,
            ec
        );

    if(
        ec == std::errc::no_such_file_or_directory ||
        status.type() == std::filesystem::file_type::not_found
    )
    {
        return true;
    }

    if(ec)
    {
        std::cerr
            << "Failed to inspect persistent prefix "
            << description
            << ":"
            << std::endl
            << "    "
            << path
            << std::endl
            << "    "
            << ec.message()
            << std::endl;

        return false;
    }

    std::error_code removeEc;

    if(
        std::filesystem::is_directory(status) &&
        !std::filesystem::is_symlink(status)
    )
    {
        std::filesystem::remove_all(
            path,
            removeEc
        );
    }
    else
    {
        std::filesystem::remove(
            path,
            removeEc
        );
    }

    if(removeEc)
    {
        std::cerr
            << "Failed to remove persistent prefix "
            << description
            << ":"
            << std::endl
            << "    "
            << path
            << std::endl
            << "    "
            << removeEc.message()
            << std::endl;

        return false;
    }

    std::cout
        << "Removed persistent prefix "
        << description
        << ":"
        << std::endl
        << "    "
        << path
        << std::endl;

    return true;
}


bool filesEqual(
    const std::filesystem::path& first,
    const std::filesystem::path& second
)
{
    std::error_code ec;

    const auto firstSize =
        std::filesystem::file_size(
            first,
            ec
        );

    if(ec)
        return false;

    const auto secondSize =
        std::filesystem::file_size(
            second,
            ec
        );

    if(ec)
        return false;

    if(firstSize != secondSize)
        return false;

    std::ifstream firstFile(
        first,
        std::ios::binary
    );

    std::ifstream secondFile(
        second,
        std::ios::binary
    );

    if(
        !firstFile ||
        !secondFile
    )
    {
        return false;
    }

    static constexpr std::size_t bufferSize =
        1024 * 1024;

    std::array<char, bufferSize> firstBuffer{};
    std::array<char, bufferSize> secondBuffer{};

    while(firstFile && secondFile)
    {
        firstFile.read(
            firstBuffer.data(),
            firstBuffer.size()
        );

        secondFile.read(
            secondBuffer.data(),
            secondBuffer.size()
        );

        const auto firstCount =
            firstFile.gcount();

        const auto secondCount =
            secondFile.gcount();

        if(firstCount != secondCount)
            return false;

        if(
            !std::equal(
                firstBuffer.begin(),
                firstBuffer.begin() + firstCount,
                secondBuffer.begin()
            )
        )
        {
            return false;
        }
    }

    return true;
}


bool removeRedundantWindowsFiles(
    const std::filesystem::path& upperWindowsDirectory,
    const std::filesystem::path& lowerWindowsDirectory
)
{
    std::error_code ec;

    const auto status =
        std::filesystem::symlink_status(
            upperWindowsDirectory,
            ec
        );

    if(
        ec == std::errc::no_such_file_or_directory ||
        status.type() == std::filesystem::file_type::not_found
    )
    {
        return true;
    }

    if(ec)
    {
        std::cerr
            << "Failed to inspect persistent Windows directory:"
            << std::endl
            << "    "
            << upperWindowsDirectory
            << std::endl
            << "    "
            << ec.message()
            << std::endl;

        return false;
    }

    if(!std::filesystem::is_directory(status))
    {
        return true;
    }

    std::filesystem::recursive_directory_iterator iterator(
        upperWindowsDirectory,
        std::filesystem::directory_options::skip_permission_denied,
        ec
    );

    if(ec)
    {
        std::cerr
            << "Failed to scan persistent Windows directory:"
            << std::endl
            << "    "
            << upperWindowsDirectory
            << std::endl
            << "    "
            << ec.message()
            << std::endl;

        return false;
    }

    const std::filesystem::recursive_directory_iterator end;

    while(iterator != end)
    {
        const auto upperPath =
            iterator->path();

        std::error_code entryError;

        const auto entryStatus =
            std::filesystem::symlink_status(
                upperPath,
                entryError
            );

        if(entryError)
        {
            std::cerr
                << "Failed to inspect persistent Windows entry:"
                << std::endl
                << "    "
                << upperPath
                << std::endl
                << "    "
                << entryError.message()
                << std::endl;

            return false;
        }

        const bool isRegularFile =
            std::filesystem::is_regular_file(entryStatus);

        const bool isSymlink =
            std::filesystem::is_symlink(entryStatus);

        if(isRegularFile || isSymlink)
        {
            const auto relativePath =
                std::filesystem::relative(
                    upperPath,
                    upperWindowsDirectory,
                    entryError
                );

            if(entryError)
            {
                std::cerr
                    << "Failed to determine persistent Windows"
                    << " relative path:"
                    << std::endl
                    << "    "
                    << upperPath
                    << std::endl
                    << "    "
                    << entryError.message()
                    << std::endl;

                return false;
            }

            const auto lowerPath =
                lowerWindowsDirectory /
                relativePath;

            std::error_code lowerError;

            const auto lowerStatus =
                std::filesystem::symlink_status(
                    lowerPath,
                    lowerError
                );

            /*
                A file is redundant if the lower prefix has the
                exact same kind of entry at the same relative path:

                    - regular files: byte-identical content.
                    - symlinks: identical link target. Most of
                      drive_c/windows/{system32,syswow64} in a Wine/
                      Proton prefix consists of symlinks into the
                      runtime's own installation (fakedlls, builtin
                      DLLs), so without this the persistent upper
                      would keep an unnecessary duplicate symlink
                      for nearly every such file forever.

                Mismatched types (e.g. upper has a real file where
                lower has a symlink, likely because the game or
                an installer replaced it) are deliberately never
                treated as redundant -- that divergence is exactly
                the kind of delta the persistent upper exists to
                keep.
            */

            bool redundant =
                false;

            if(
                isRegularFile &&
                !lowerError &&
                std::filesystem::is_regular_file(lowerStatus)
            )
            {
                redundant =
                    filesEqual(
                        upperPath,
                        lowerPath
                    );
            }
            else if(
                isSymlink &&
                !lowerError &&
                std::filesystem::is_symlink(lowerStatus)
            )
            {
                std::error_code upperLinkError;
                std::error_code lowerLinkError;

                const auto upperTarget =
                    std::filesystem::read_symlink(
                        upperPath,
                        upperLinkError
                    );

                const auto lowerTarget =
                    std::filesystem::read_symlink(
                        lowerPath,
                        lowerLinkError
                    );

                redundant =
                    !upperLinkError &&
                    !lowerLinkError &&
                    upperTarget == lowerTarget;
            }

            if(redundant)
            {
                std::filesystem::remove(
                    upperPath,
                    entryError
                );

                if(entryError)
                {
                    std::cerr
                        << "Failed to remove redundant persistent"
                        << " Windows file:"
                        << std::endl
                        << "    "
                        << upperPath
                        << std::endl
                        << "    "
                        << entryError.message()
                        << std::endl;

                    return false;
                }

                std::cout
                    << "Removed redundant persistent Windows "
                    << (isSymlink ? "symlink" : "file")
                    << ":"
                    << std::endl
                    << "    "
                    << upperPath
                    << std::endl;
            }
        }

        ++iterator;
    }

    return true;
}

} // namespace


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