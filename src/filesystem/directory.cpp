#include "filesystem_internal.hpp"

#include <filesystem>
#include <iostream>
#include <system_error>


/*
    ================================================================
    REMOVE DIRECTORY
    ================================================================
*/

bool removeDirectory(
    const std::filesystem::path& path
)
{
    if(path.empty())
    {
        return true;
    }

    std::error_code ec;

    /*
        Use symlink_status here.

        std::filesystem::exists() follows symlinks and can therefore
        itself encounter a broken/recursive symlink.
    */

    const auto status =
        std::filesystem::symlink_status(
            path,
            ec
        );

    if(ec)
    {
        return true;
    }

    if(
        std::filesystem::is_symlink(status) ||
        std::filesystem::is_regular_file(status)
    )
    {
        std::filesystem::remove(
            path,
            ec
        );

        return !ec;
    }

    if(
        !std::filesystem::is_directory(status)
    )
    {
        return true;
    }

    std::filesystem::remove_all(
        path,
        ec
    );

    if(ec)
    {
        std::cerr
            << "Could not remove:"
            << std::endl
            << "    "
            << path
            << std::endl
            << "    "
            << ec.message()
            << std::endl;

        return false;
    }

    return true;
}


/*
    ================================================================
    REPLICATE DIRECTORY STRUCTURE
    ================================================================

    IMPORTANT:

    Never use std::filesystem::relative() here.

    std::filesystem::relative() may resolve symlinks.

    Wine/game installations can contain valid symlinks and, in the
    damaged-prefix case, recursive symlinks.

    We therefore calculate the path purely lexically.
*/

bool replicateDirectoryStructure(
    const std::filesystem::path& source,
    const std::filesystem::path& destination
)
{
    std::error_code ec;

    const auto sourceStatus =
        std::filesystem::symlink_status(
            source,
            ec
        );

    if(
        ec ||
        !std::filesystem::is_directory(
            sourceStatus
        )
    )
    {
        std::cerr
            << "Source directory does not exist:"
            << std::endl
            << "    "
            << source
            << std::endl;

        return false;
    }


    try
    {
        std::filesystem::create_directories(
            destination
        );


        for(
            const auto& entry :
            std::filesystem::recursive_directory_iterator(
                source,
                std::filesystem::directory_options::
                    skip_permission_denied
            )
        )
        {
            std::error_code entryError;

            const auto status =
                entry.symlink_status(
                    entryError
                );

            if(entryError)
            {
                std::cerr
                    << "Could not inspect directory entry:"
                    << std::endl
                    << "    "
                    << entry.path()
                    << std::endl
                    << "    "
                    << entryError.message()
                    << std::endl;

                return false;
            }


            /*
                Only replicate real directories.

                Symlinks are deliberately ignored because this
                function creates only the upper directory structure.
            */

            if(
                !std::filesystem::is_directory(status)
            )
            {
                continue;
            }


            /*
                PURELY LEXICAL RELATIVE PATH.

                This does NOT follow symlinks.
            */

            const auto relative =
                entry.path().lexically_relative(
                    source
                );


            if(
                relative.empty() ||
                relative == "."
            )
            {
                continue;
            }


            const auto target =
                destination /
                relative;


            std::filesystem::create_directories(
                target
            );
        }
    }
    catch(const std::exception& e)
    {
        std::cerr
            << "Could not replicate GameData directory structure:"
            << std::endl
            << "    "
            << e.what()
            << std::endl;

        return false;
    }


    return true;
}