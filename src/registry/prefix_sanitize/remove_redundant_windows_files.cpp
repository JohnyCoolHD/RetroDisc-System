#include "prefix_sanitize_internal.hpp"

#include <filesystem>
#include <iostream>
#include <system_error>


namespace prefix_sanitize_internal
{


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


} // namespace prefix_sanitize_internal
