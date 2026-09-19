#include "runtime_internal.hpp"

#include <filesystem>
#include <iostream>
#include <system_error>


namespace runtime_internal
{


/*
    ================================================================
    ENSURE DIRECTORY
    ================================================================
*/

bool ensureDirectory(
    const std::filesystem::path& directory
)
{
    if(directory.empty())
    {
        return false;
    }

    /*
        First inspect the entry itself. This handles broken symlinks
        correctly.
    */

    if(!removeBrokenOrInvalidEntry(directory))
    {
        return false;
    }

    std::error_code ec;

    if(
        std::filesystem::is_directory(
            directory,
            ec
        )
    )
    {
        return true;
    }

    /*
        create_directories() is used rather than create_directory()
        because parent directories may also be missing.
    */

    std::filesystem::create_directories(
        directory,
        ec
    );

    if(ec)
    {
        /*
            Race-safe second check.

            Another process may have created the directory between
            our checks.
        */

        std::error_code verifyEc;

        if(
            std::filesystem::is_directory(
                directory,
                verifyEc
            )
        )
        {
            return true;
        }

        std::cerr
            << "Could not create directory:"
            << std::endl
            << "    "
            << directory
            << std::endl
            << "Error:"
            << std::endl
            << "    "
            << ec.message()
            << std::endl;

        return false;
    }

    return true;
}


} // namespace runtime_internal
