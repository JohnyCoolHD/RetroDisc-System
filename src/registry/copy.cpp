#include "registry_internal.hpp"

#include <filesystem>
#include <iostream>
#include <system_error>


/*
    ================================================================
    COPY PREFIX ENTRY
    ================================================================

    Symlinks are copied as symlinks.

    Their targets are NEVER followed.
    ================================================================
*/

bool copyPrefixEntry(
    const std::filesystem::path& source,
    const std::filesystem::path& destination
)
{
    std::error_code ec;


    const auto status =
        std::filesystem::symlink_status(
            source,
            ec
        );


    if(ec)
    {
        std::cerr
            << "Could not inspect prefix entry:"
            << std::endl
            << "    "
            << source
            << std::endl
            << "    "
            << ec.message()
            << std::endl;

        return false;
    }


    /*
        ============================================================
        SYMLINK
        ============================================================
    */

    if(std::filesystem::is_symlink(
        status
    ))
    {
        const auto target =
            std::filesystem::read_symlink(
                source,
                ec
            );


        if(ec)
        {
            std::cerr
                << "Could not read prefix symlink:"
                << std::endl
                << "    "
                << source
                << std::endl
                << "    "
                << ec.message()
                << std::endl;

            return false;
        }


        const auto destinationStatus =
            std::filesystem::symlink_status(
                destination,
                ec
            );


        if(
            !ec &&
            (
                std::filesystem::exists(
                    destinationStatus
                ) ||
                std::filesystem::is_symlink(
                    destinationStatus
                )
            )
        )
        {
            ec.clear();

            std::filesystem::remove(
                destination,
                ec
            );


            if(ec)
            {
                std::cerr
                    << "Could not replace prefix symlink:"
                    << std::endl
                    << "    "
                    << destination
                    << std::endl
                    << "    "
                    << ec.message()
                    << std::endl;

                return false;
            }
        }


        std::filesystem::create_symlink(
            target,
            destination,
            ec
        );


        if(ec)
        {
            std::cerr
                << "Could not create prefix symlink:"
                << std::endl
                << "    "
                << destination
                << std::endl
                << "Target:"
                << std::endl
                << "    "
                << target
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


    /*
        ============================================================
        DIRECTORY
        ============================================================
    */

    if(std::filesystem::is_directory(
        status
    ))
    {
        const auto destinationStatus =
            std::filesystem::symlink_status(
                destination,
                ec
            );


        if(
            !ec &&
            std::filesystem::is_symlink(
                destinationStatus
            )
        )
        {
            std::filesystem::remove(
                destination,
                ec
            );


            if(ec)
            {
                std::cerr
                    << "Could not remove destination symlink:"
                    << std::endl
                    << "    "
                    << destination
                    << std::endl
                    << "    "
                    << ec.message()
                    << std::endl;

                return false;
            }
        }


        if(
            !std::filesystem::exists(
                destination,
                ec
            )
        )
        {
            ec.clear();

            std::filesystem::create_directory(
                destination,
                ec
            );


            if(ec)
            {
                std::cerr
                    << "Could not create prefix directory:"
                    << std::endl
                    << "    "
                    << destination
                    << std::endl
                    << "    "
                    << ec.message()
                    << std::endl;

                return false;
            }
        }


        std::filesystem::directory_iterator iterator(
            source,
            std::filesystem::directory_options::
                skip_permission_denied,
            ec
        );


        if(ec)
        {
            std::cerr
                << "Could not read prefix directory:"
                << std::endl
                << "    "
                << source
                << std::endl
                << "    "
                << ec.message()
                << std::endl;

            return false;
        }


        for(
            const auto& entry :
            iterator
        )
        {
            const auto target =
                destination /
                entry.path().filename();


            if(!copyPrefixEntry(
                entry.path(),
                target
            ))
            {
                return false;
            }
        }


        return true;
    }


    /*
        ============================================================
        REGULAR FILE
        ============================================================
    */

    if(std::filesystem::is_regular_file(
        status
    ))
    {
        std::filesystem::copy_file(
            source,
            destination,
            std::filesystem::copy_options::
                overwrite_existing,
            ec
        );


        if(ec)
        {
            std::cerr
                << "Could not copy prefix file:"
                << std::endl
                << "    "
                << source
                << std::endl
                << "to:"
                << std::endl
                << "    "
                << destination
                << std::endl
                << "    "
                << ec.message()
                << std::endl;

            return false;
        }


        return true;
    }


    /*
        ============================================================
        UNKNOWN TYPE
        ============================================================
    */

    std::cerr
        << "Unsupported filesystem entry in Wine prefix:"
        << std::endl
        << "    "
        << source
        << std::endl;


    return false;
}


/*
    ================================================================
    COPY WINE PREFIX
    ================================================================
*/

bool copyWinePrefix(
    const std::filesystem::path& source,
    const std::filesystem::path& destination
)
{
    if(!prefixLooksValid(
        source
    ))
    {
        std::cerr
            << "Bundled Wine prefix is invalid or incomplete:"
            << std::endl
            << "    "
            << source
            << std::endl;

        std::cerr
            << "Required:"
            << std::endl
            << "    drive_c/"
            << std::endl
            << "    dosdevices/"
            << std::endl
            << "    system.reg"
            << std::endl
            << "    user.reg"
            << std::endl;

        return false;
    }


    std::error_code ec;


    const auto destinationStatus =
        std::filesystem::symlink_status(
            destination,
            ec
        );


    if(
        !ec &&
        (
            std::filesystem::exists(
                destinationStatus
            ) ||
            std::filesystem::is_symlink(
                destinationStatus
            )
        )
    )
    {
        std::cerr
            << "Refusing to overwrite existing Wine prefix:"
            << std::endl
            << "    "
            << destination
            << std::endl;

        return false;
    }


    std::filesystem::create_directories(
        destination,
        ec
    );


    if(ec)
    {
        std::cerr
            << "Could not create Wine prefix destination:"
            << std::endl
            << "    "
            << destination
            << std::endl
            << "    "
            << ec.message()
            << std::endl;

        return false;
    }


    std::cout
        << "Copying bundled Wine prefix:"
        << std::endl
        << "    Source:"
        << std::endl
        << "        "
        << source
        << std::endl
        << "    Destination:"
        << std::endl
        << "        "
        << destination
        << std::endl;


    std::filesystem::directory_iterator iterator(
        source,
        std::filesystem::directory_options::
            skip_permission_denied,
        ec
    );


    if(ec)
    {
        std::cerr
            << "Could not read bundled Wine prefix:"
            << std::endl
            << "    "
            << source
            << std::endl
            << "    "
            << ec.message()
            << std::endl;

        return false;
    }


    for(
        const auto& entry :
        iterator
    )
    {
        const auto target =
            destination /
            entry.path().filename();


        if(!copyPrefixEntry(
            entry.path(),
            target
        ))
        {
            std::cerr
                << "Failed while copying:"
                << std::endl
                << "    "
                << entry.path()
                << std::endl;

            return false;
        }
    }


    if(!prefixLooksValid(
        destination
    ))
    {
        std::cerr
            << "Copied Wine prefix is incomplete:"
            << std::endl
            << "    "
            << destination
            << std::endl;

        return false;
    }


    std::cout
        << "Bundled Wine prefix copied successfully."
        << std::endl;


    return true;
}