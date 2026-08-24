#include "registry.hpp"
#include "context.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <system_error>


namespace
{


std::filesystem::path getHome()
{
    const char* home =
        std::getenv("HOME");

    if(
        home == nullptr ||
        *home == '\0'
    )
    {
        return {};
    }

    return std::filesystem::path(home);
}


/*
    ================================================================
    DIRECTORY EXISTS
    ================================================================
*/

bool directoryExists(
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
        return false;
    }

    return std::filesystem::is_directory(
        status
    );
}


/*
    ================================================================
    REGULAR FILE EXISTS
    ================================================================
*/

bool regularFileExists(
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
        return false;
    }

    return std::filesystem::is_regular_file(
        status
    );
}


/*
    ================================================================
    PREFIX VALIDATION
    ================================================================

    A Wine prefix does NOT need a "version" file to be usable.

    Required structure:

        pfx/
        ├── drive_c/
        ├── dosdevices/
        ├── system.reg
        └── user.reg

    Proton prefixes use the same basic Wine prefix structure.

    We deliberately do NOT require:

        pfx/version

    because that made otherwise valid prefixes appear missing.
    ================================================================
*/

bool prefixLooksValid(
    const std::filesystem::path& prefix
)
{
    if(!directoryExists(prefix))
    {
        return false;
    }


    const auto driveC =
        prefix /
        "drive_c";


    const auto dosDevices =
        prefix /
        "dosdevices";


    const auto systemReg =
        prefix /
        "system.reg";


    const auto userReg =
        prefix /
        "user.reg";


    if(!directoryExists(
        driveC
    ))
    {
        return false;
    }


    if(!directoryExists(
        dosDevices
    ))
    {
        return false;
    }


    if(!regularFileExists(
        systemReg
    ))
    {
        return false;
    }


    if(!regularFileExists(
        userReg
    ))
    {
        return false;
    }


    return true;
}


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


} // namespace


/*
    ================================================================
    GLOBAL PREFIX
    ================================================================
*/

bool prepareGlobalPrefix(
    Context& ctx
)
{
    const auto home =
        getHome();


    if(home.empty())
    {
        std::cerr
            << "Could not determine HOME."
            << std::endl;

        return false;
    }


    ctx.globalPrefixDirectory =
        home /
        ".RetroDisc";


    std::error_code ec;

    std::filesystem::create_directories(
        ctx.globalPrefixDirectory,
        ec
    );


    if(ec)
    {
        std::cerr
            << "Could not create global RetroDisc directory:"
            << std::endl
            << "    "
            << ctx.globalPrefixDirectory
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
    LOCAL PREFIX
    ================================================================
*/

bool prepareLocalPrefix(
    Context& ctx
)
{
    const auto home =
        getHome();


    if(home.empty())
    {
        std::cerr
            << "Could not determine HOME."
            << std::endl;

        return false;
    }


    std::filesystem::path localGameDirectory;


    if(!ctx.dataPath.empty())
    {
        localGameDirectory =
            ctx.dataPath;
    }
    else
    {
        localGameDirectory =
            home /
            "Games" /
            "RetroDisc" /
            ctx.gameId;
    }


    const auto localPrefix =
        localGameDirectory /
        "pfx";


    ctx.prefixOverlayDirectory =
        localPrefix;


    try
    {
        std::filesystem::create_directories(
            localGameDirectory
        );
    }
    catch(const std::exception& e)
    {
        std::cerr
            << "Could not create local game directory:"
            << std::endl
            << "    "
            << localGameDirectory
            << std::endl
            << e.what()
            << std::endl;

        return false;
    }


    /*
        ============================================================
        EXISTING PREFIX
        ============================================================
    */

    if(prefixLooksValid(
        localPrefix
    ))
    {
        std::cout
            << "Persistent game Wine prefix:"
            << std::endl
            << "    "
            << localPrefix
            << std::endl;

        return true;
    }


    /*
        ============================================================
        EXISTING BUT INVALID PREFIX
        ============================================================
    */

    if(directoryExists(
        localPrefix
    ))
    {
        std::cerr
            << "Persistent game prefix exists but is incomplete:"
            << std::endl
            << "    "
            << localPrefix
            << std::endl;

        std::cerr
            << "Expected:"
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


    /*
        ============================================================
        BUNDLED PREFIX
        ============================================================
    */

    const auto bundledPrefix =
        ctx.root /
        "pfx";


    if(prefixLooksValid(
        bundledPrefix
    ))
    {
        std::cout
            << "No persistent game prefix found."
            << std::endl;


        std::cout
            << "Bundled prefix found:"
            << std::endl
            << "    "
            << bundledPrefix
            << std::endl;


        if(!copyWinePrefix(
            bundledPrefix,
            localPrefix
        ))
        {
            std::cerr
                << "Could not copy bundled Wine prefix."
                << std::endl;

            return false;
        }


        return true;
    }


    /*
        ============================================================
        NO PREFIX
        ============================================================
    */

    std::cerr
        << "No Wine prefix found."
        << std::endl
        << "Persistent:"
        << std::endl
        << "    "
        << localPrefix
        << std::endl
        << "Bundled:"
        << std::endl
        << "    "
        << bundledPrefix
        << std::endl;


    return false;
}


/*
    ================================================================
    PREPARE PREFIX
    ================================================================
*/

bool preparePrefix(
    Context& ctx
)
{
    if(!prepareGlobalPrefix(
        ctx
    ))
    {
        return false;
    }


    if(!prepareLocalPrefix(
        ctx
    ))
    {
        return false;
    }


    ctx.prefixOverlayMounted =
        false;


    std::cout
        << std::endl
        << "================================================"
        << std::endl
        << "WINE PREFIX"
        << std::endl
        << "================================================"
        << std::endl;


    std::cout
        << "Prefix:"
        << std::endl
        << "    "
        << ctx.prefixOverlayDirectory
        << std::endl;


    std::cout
        << "Prefix overlay: disabled."
        << std::endl;


    return true;
}