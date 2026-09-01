#include "registry_internal.hpp"

#include "filesystem.hpp"

#include <filesystem>
#include <iostream>
#include <string>


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


    std::filesystem::recursive_directory_iterator iterator(
        source,
        std::filesystem::directory_options::skip_permission_denied,
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


    const auto end =
        std::filesystem::recursive_directory_iterator();


    while(iterator != end)
    {
        const auto sourcePath =
            iterator->path();


        /*
            Build the destination path purely lexically.

            We deliberately do not use std::filesystem::relative(),
            because that function can resolve symbolic links.

            The iterator path itself is already rooted at `source`.
            Removing the source prefix keeps symbolic links intact.
        */


        std::string sourceString =
            sourcePath.generic_string();


        const std::string sourceRoot =
            source.generic_string();


        if(
            sourceString.size() <=
            sourceRoot.size()
        )
        {
            std::cerr
                << "Could not determine relative Wine prefix path:"
                << std::endl
                << "    "
                << sourcePath
                << std::endl;

            return false;
        }


        std::string relativeString =
            sourceString.substr(
                sourceRoot.size()
            );


        while(
            !relativeString.empty() &&
            relativeString.front() == '/'
        )
        {
            relativeString.erase(
                relativeString.begin()
            );
        }


        if(relativeString.empty())
        {
            ++iterator;
            continue;
        }


        const std::filesystem::path relativePath =
            std::filesystem::path(
                relativeString
            );


        /*
            ============================================================
            SKIP drive_c/windows
            ============================================================
        */


        if(
            relativeString ==
                "drive_c/windows" ||
            relativeString.rfind(
                "drive_c/windows/",
                0
            ) == 0
        )
        {
            std::cout
                << "Skipping bundled Wine Windows directory:"
                << std::endl
                << "    "
                << sourcePath
                << std::endl;


            std::error_code directoryError;


            if(iterator->is_directory(
                directoryError
            ))
            {
                iterator.disable_recursion_pending();
            }


            ++iterator;

            continue;
        }


        const auto target =
            destination /
            relativePath;


        /*
            ============================================================
            SYMLINKS
            ============================================================
        */


        std::error_code entryError;


        if(iterator->is_symlink(
            entryError
        ))
        {
            if(entryError)
            {
                std::cerr
                    << "Could not inspect Wine prefix symlink:"
                    << std::endl
                    << "    "
                    << sourcePath
                    << std::endl
                    << "    "
                    << entryError.message()
                    << std::endl;

                return false;
            }


            const auto linkTarget =
                std::filesystem::read_symlink(
                    sourcePath,
                    entryError
                );


            if(entryError)
            {
                std::cerr
                    << "Could not read Wine prefix symlink:"
                    << std::endl
                    << "    "
                    << sourcePath
                    << std::endl
                    << "    "
                    << entryError.message()
                    << std::endl;

                return false;
            }


            /*
                Ignore links pointing into /home.

                These are user-specific links from the bundled prefix.
            */


            if(linkTarget.is_absolute())
            {
                const auto normalizedTarget =
                    linkTarget.lexically_normal();


                if(
                    normalizedTarget ==
                        std::filesystem::path("/home") ||
                    normalizedTarget.string().rfind(
                        "/home/",
                        0
                    ) == 0
                )
                {
                    std::cout
                        << "Skipping personal Wine prefix symlink:"
                        << std::endl
                        << "    "
                        << sourcePath
                        << " -> "
                        << linkTarget
                        << std::endl;

                    ++iterator;

                    continue;
                }
            }


            /*
                Copy all other symlinks unchanged.

                This includes:

                    dosdevices/c:  -> ../drive_c
                    dosdevices/z:  -> /
                    dosdevices/com1 -> /dev/ttyS0
                    etc.
            */


            std::filesystem::create_directories(
                target.parent_path(),
                entryError
            );


            if(entryError)
            {
                std::cerr
                    << "Could not create Wine prefix symlink parent:"
                    << std::endl
                    << "    "
                    << target.parent_path()
                    << std::endl
                    << "    "
                    << entryError.message()
                    << std::endl;

                return false;
            }


            std::filesystem::create_symlink(
                linkTarget,
                target,
                entryError
            );


            if(entryError)
            {
                std::cerr
                    << "Could not create Wine prefix symlink:"
                    << std::endl
                    << "    "
                    << target
                    << std::endl
                    << "    "
                    << entryError.message()
                    << std::endl;

                return false;
            }


            ++iterator;

            continue;
        }


        /*
            ============================================================
            DIRECTORIES
            ============================================================
        */


        entryError.clear();


        if(iterator->is_directory(
            entryError
        ))
        {
            if(entryError)
            {
                std::cerr
                    << "Could not inspect Wine prefix directory:"
                    << std::endl
                    << "    "
                    << sourcePath
                    << std::endl
                    << "    "
                    << entryError.message()
                    << std::endl;

                return false;
            }


            std::filesystem::create_directories(
                target,
                entryError
            );


            if(entryError)
            {
                std::cerr
                    << "Could not create Wine prefix directory:"
                    << std::endl
                    << "    "
                    << target
                    << std::endl
                    << "    "
                    << entryError.message()
                    << std::endl;

                return false;
            }


            ++iterator;

            continue;
        }


        /*
            ============================================================
            REGULAR FILES
            ============================================================
        */


        entryError.clear();


        if(iterator->is_regular_file(
            entryError
        ))
        {
            if(entryError)
            {
                std::cerr
                    << "Could not inspect Wine prefix file:"
                    << std::endl
                    << "    "
                    << sourcePath
                    << std::endl
                    << "    "
                    << entryError.message()
                    << std::endl;

                return false;
            }


            std::filesystem::create_directories(
                target.parent_path(),
                entryError
            );


            if(entryError)
            {
                std::cerr
                    << "Could not create Wine prefix file parent:"
                    << std::endl
                    << "    "
                    << target.parent_path()
                    << std::endl
                    << "    "
                    << entryError.message()
                    << std::endl;

                return false;
            }


            std::filesystem::copy_file(
                sourcePath,
                target,
                std::filesystem::copy_options::none,
                entryError
            );


            if(entryError)
            {
                std::cerr
                    << "Could not copy Wine prefix file:"
                    << std::endl
                    << "    "
                    << sourcePath
                    << std::endl
                    << "to:"
                    << std::endl
                    << "    "
                    << target
                    << std::endl
                    << "    "
                    << entryError.message()
                    << std::endl;

                return false;
            }


            ++iterator;

            continue;
        }


        /*
            ============================================================
            UNKNOWN FILE TYPE
            ============================================================
        */


        std::cerr
            << "Unsupported Wine prefix filesystem entry:"
            << std::endl
            << "    "
            << sourcePath
            << std::endl;

        return false;
    }


    std::cout
        << "Bundled Wine prefix copied successfully."
        << std::endl;


    return true;
}