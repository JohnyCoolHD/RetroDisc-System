#include "prefix_internal.hpp"
#include "../registry_internal.hpp"

#include <filesystem>
#include <iostream>
#include <system_error>


namespace prefix_internal
{


bool copyBundledPrefix(
    const std::filesystem::path& source,
    const std::filesystem::path& destination,
    const std::filesystem::path& base
)
{
    std::error_code ec;


    const auto sourceDriveC =
        source / "drive_c";

    const auto sourceDosDevices =
        source / "dosdevices";

    const auto sourceSystemReg =
        source / "system.reg";

    const auto sourceUserReg =
        source / "user.reg";


    if(
        !directoryExists(sourceDriveC) ||
        !directoryExists(sourceDosDevices) ||
        !regularFileExists(sourceSystemReg) ||
        !regularFileExists(sourceUserReg)
    )
    {
        std::cerr
            << "Bundled Wine prefix is invalid or incomplete:"
            << std::endl
            << "    "
            << source
            << std::endl;

        return false;
    }


    const auto baseRoot =
        (base / "pfx").lexically_normal();


    if(
        !directoryExists(
            baseRoot / "drive_c"
        ) ||
        !directoryExists(
            baseRoot / "dosdevices"
        ) ||
        !regularFileExists(
            baseRoot / "system.reg"
        ) ||
        !regularFileExists(
            baseRoot / "user.reg"
        )
    )
    {
        std::cerr
            << "Selected global base prefix is invalid or incomplete:"
            << std::endl
            << "    "
            << baseRoot
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
        std::cerr
            << "Wine prefix delta destination already exists:"
            << std::endl
            << "    "
            << destination
            << std::endl;

        return false;
    }


    if(ec != std::errc::no_such_file_or_directory)
    {
        std::cerr
            << "Could not inspect Wine prefix delta destination:"
            << std::endl
            << "    "
            << destination
            << std::endl
            << "    "
            << ec.message()
            << std::endl;

        return false;
    }


    ec.clear();


    std::filesystem::create_directories(
        destination,
        ec
    );


    if(ec)
    {
        std::cerr
            << "Could not create Wine prefix delta destination:"
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
        << "Creating bundled Wine prefix delta:"
        << std::endl
        << "    Source: "
        << source
        << std::endl
        << "    Base:   "
        << base
        << std::endl
        << "    Base root:"
        << std::endl
        << "        "
        << baseRoot
        << std::endl
        << "    Upper:  "
        << destination
        << std::endl;


    const auto sourceRoot =
        source.lexically_normal();


    std::filesystem::recursive_directory_iterator iterator(
        source,
        std::filesystem::directory_options::skip_permission_denied,
        ec
    );


    if(ec)
    {
        std::cerr
            << "Could not iterate bundled Wine prefix:"
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


        const auto relative =
            sourcePath.lexically_relative(
                sourceRoot
            );


        const auto target =
            destination / relative;


        const auto basePath =
            baseRoot / relative;


        std::error_code entryError;


        const auto status =
            std::filesystem::symlink_status(
                sourcePath,
                entryError
            );


        if(entryError)
        {
            std::cerr
                << "Could not inspect bundled Wine prefix entry:"
                << std::endl
                << "    "
                << sourcePath
                << std::endl
                << "    "
                << entryError.message()
                << std::endl;

            return false;
        }


        if(std::filesystem::is_symlink(status))
        {
            const auto linkTarget =
                std::filesystem::read_symlink(
                    sourcePath,
                    entryError
                );


            if(entryError)
            {
                std::cerr
                    << "Could not read bundled Wine prefix symlink:"
                    << std::endl
                    << "    "
                    << sourcePath
                    << std::endl
                    << "    "
                    << entryError.message()
                    << std::endl;

                return false;
            }


            if(isPersonalHomeSymlink(
                   sourcePath,
                   linkTarget
               ))
            {
                ++iterator;
                continue;
            }


            bool same = false;


            const auto baseStatus =
                std::filesystem::symlink_status(
                    basePath,
                    entryError
                );


            if(
                !entryError &&
                std::filesystem::is_symlink(
                    baseStatus
                )
            )
            {
                std::error_code targetError;


                const auto existingTarget =
                    std::filesystem::read_symlink(
                        basePath,
                        targetError
                    );


                same =
                    !targetError &&
                    existingTarget == linkTarget;
            }


            if(!same)
            {
                std::filesystem::create_directories(
                    target.parent_path(),
                    entryError
                );


                if(entryError)
                {
                    std::cerr
                        << "Could not create parent directory for"
                        << " bundled Wine prefix symlink:"
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
                        << "Could not create bundled Wine prefix symlink:"
                        << std::endl
                        << "    "
                        << target
                        << std::endl
                        << "    "
                        << entryError.message()
                        << std::endl;

                    return false;
                }
            }


            ++iterator;
            continue;
        }


        if(std::filesystem::is_directory(status))
        {
            const auto baseStatus =
                std::filesystem::symlink_status(
                    basePath,
                    entryError
                );


            if(
                !entryError &&
                std::filesystem::is_directory(
                    baseStatus
                )
            )
            {
                ++iterator;
                continue;
            }


            std::filesystem::create_directories(
                target,
                entryError
            );


            if(entryError)
            {
                std::cerr
                    << "Could not create bundled Wine prefix"
                    << " directory:"
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


        if(std::filesystem::is_regular_file(status))
        {
            if(filesEqual(
                   sourcePath,
                   basePath
               ))
            {
                ++iterator;
                continue;
            }


            std::filesystem::create_directories(
                target.parent_path(),
                entryError
            );


            if(entryError)
            {
                std::cerr
                    << "Could not create parent directory for"
                    << " bundled Wine prefix file:"
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
                    << "Could not copy bundled Wine prefix file:"
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


        std::cerr
            << "Unsupported bundled Wine prefix entry:"
            << std::endl
            << "    "
            << sourcePath
            << std::endl;

        return false;
    }


    std::cout
        << "Bundled Wine prefix delta created successfully."
        << std::endl;


    return true;
}


} // namespace prefix_internal
