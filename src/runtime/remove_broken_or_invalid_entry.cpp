#include "runtime_internal.hpp"

#include <filesystem>
#include <iostream>
#include <system_error>


namespace runtime_internal
{


/*
    ================================================================
    FILESYSTEM ENTRY HELPERS
    ================================================================
*/

/*
    symlink_status() is deliberately used here instead of
    is_directory().

    A broken symlink returns a valid symlink_status() entry, but
    is_directory() on the symlink itself returns false.

    This is exactly the situation that caused:

        Existing symlink does not resolve to a directory

    We repair such entries instead of aborting the launch.
*/

bool removeBrokenOrInvalidEntry(
    const std::filesystem::path& path
)
{
    std::error_code ec;

    const auto status =
        std::filesystem::symlink_status(
            path,
            ec
        );

    if(ec)
    {
        /*
            If the entry does not exist, there is nothing to remove.
        */

        if(ec == std::errc::no_such_file_or_directory)
        {
            return true;
        }

        std::cerr
            << "Could not inspect filesystem entry:"
            << std::endl
            << "    "
            << path
            << std::endl
            << "Error:"
            << std::endl
            << "    "
            << ec.message()
            << std::endl;

        return false;
    }

    if(
        std::filesystem::is_symlink(status)
    )
    {
        /*
            Check whether the symlink resolves to a directory.

            filesystem::is_directory(path) follows the symlink.
        */

        std::error_code targetEc;

        const bool targetIsDirectory =
            std::filesystem::is_directory(
                path,
                targetEc
            );

        if(
            !targetEc &&
            targetIsDirectory
        )
        {
            return true;
        }

        /*
            Broken symlink or symlink to something that is not a
            directory.

            Remove it so the canonical directory can be recreated.
        */

        std::cout
            << "Removing invalid canonical symlink:"
            << std::endl
            << "    "
            << path
            << std::endl;

        std::error_code removeEc;

        std::filesystem::remove(
            path,
            removeEc
        );

        if(removeEc)
        {
            std::cerr
                << "Could not remove invalid symlink:"
                << std::endl
                << "    "
                << path
                << std::endl
                << "Error:"
                << std::endl
                << "    "
                << removeEc.message()
                << std::endl;

            return false;
        }

        return true;
    }

    /*
        Existing regular directory is valid.
    */

    if(
        std::filesystem::is_directory(status)
    )
    {
        return true;
    }

    /*
        A regular file at a location where a directory is required
        cannot be used.
    */

    std::cerr
        << "Canonical path is not a directory:"
        << std::endl
        << "    "
        << path
        << std::endl;

    return false;
}


} // namespace runtime_internal
