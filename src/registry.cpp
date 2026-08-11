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

    if(home == nullptr)
    {
        return {};
    }

    return std::filesystem::path(home);
}

bool directoryExists(
    const std::filesystem::path& path
)
{
    std::error_code ec;

    return
        std::filesystem::exists(
            path,
            ec
        ) &&
        std::filesystem::is_directory(
            path,
            ec
        );
}

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

    const auto systemReg =
        prefix /
        "system.reg";

    const auto userReg =
        prefix /
        "user.reg";

    const auto dosDevices =
        prefix /
        "dosdevices";

    return
        std::filesystem::is_directory(
            driveC
        ) &&
        std::filesystem::is_directory(
            dosDevices
        ) &&
        (
            std::filesystem::exists(
                systemReg
            ) ||
            std::filesystem::exists(
                userReg
            )
        );
}


/*
    ================================================================
    COPY WINE PREFIX
    ================================================================

    Important:

    Wine prefixes contain many symlinks, especially inside:

        dosdevices/

    std::filesystem::copy() is deliberately not used here because
    different libstdc++ / filesystem implementations can follow
    or mishandle these links.

    We explicitly copy:

        directories
        regular files
        symlinks

    Symlink targets are copied exactly as stored in the source.
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
        ------------------------------------------------------------
        SYMLINK
        ------------------------------------------------------------
    */

    if(std::filesystem::is_symlink(status))
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

        /*
            If something already exists at the destination,
            remove it before creating the symlink.

            This is safe because the destination prefix is only
            created when the persistent prefix does not exist.
        */
        if(
            std::filesystem::exists(
                destination,
                ec
            ) ||
            std::filesystem::is_symlink(
                destination,
                ec
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
        ------------------------------------------------------------
        DIRECTORY
        ------------------------------------------------------------
    */

    if(std::filesystem::is_directory(status))
    {
        if(!std::filesystem::exists(
            destination,
            ec
        ))
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

        for(
            const auto& entry :
            std::filesystem::directory_iterator(
                source,
                std::filesystem::directory_options::
                    skip_permission_denied,
                ec
            )
        )
        {
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

            const auto childDestination =
                destination /
                entry.path().filename();

            if(!copyPrefixEntry(
                entry.path(),
                childDestination
            ))
            {
                return false;
            }
        }

        return true;
    }

    /*
        ------------------------------------------------------------
        REGULAR FILE
        ------------------------------------------------------------
    */

    if(std::filesystem::is_regular_file(status))
    {
        /*
            copy_file() is used instead of copy().
        */

        std::filesystem::copy_file(
            source,
            destination,
            std::filesystem::copy_options::overwrite_existing,
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
        ------------------------------------------------------------
        OTHER FILE TYPE
        ------------------------------------------------------------
    */

    std::cerr
        << "Unsupported filesystem entry in Wine prefix:"
        << std::endl
        << "    "
        << source
        << std::endl;

    return false;
}


bool copyWinePrefix(
    const std::filesystem::path& source,
    const std::filesystem::path& destination
)
{
    std::error_code ec;

    if(!directoryExists(source))
    {
        std::cerr
            << "Bundled Wine prefix is not a directory:"
            << std::endl
            << "    "
            << source
            << std::endl;

        return false;
    }

    /*
        Destination must not already be a prefix.

        prepareLocalPrefix() only calls us when it does not exist,
        but we protect against accidental reuse here as well.
    */

    if(
        std::filesystem::exists(
            destination,
            ec
        ) ||
        std::filesystem::is_symlink(
            destination,
            ec
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

    for(
        const auto& entry :
        std::filesystem::directory_iterator(
            source,
            std::filesystem::directory_options::
                skip_permission_denied,
            ec
        )
    )
    {
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

    /*
        Verify the resulting prefix before returning.
    */

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

    /*
        The global prefix is no longer used as an overlay.

        Keep the field populated for compatibility with the
        existing Context structure.
    */

    ctx.globalPrefixDirectory =
        home /
        ".RetroDisc";

    /*
        We no longer create or use this directory as a Wine prefix.

        It can still exist from older RetroDisc versions.
    */

    return true;
}


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

    /*
        Persistent game directory:

            ~/Games/RetroDisc/<gameId>/

        Persistent Wine prefix:

            ~/Games/RetroDisc/<gameId>/pfx
    */

    const auto localGameDirectory =
        home /
        "Games" /
        "RetroDisc" /
        ctx.gameId;

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
        EXISTING COMPLETE PREFIX
        ============================================================

        This is the normal case after the first launch.

        NEVER overwrite it.
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
        Existing but incomplete prefix.

        Do not destroy it automatically.
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
            << "    system.reg or user.reg"
            << std::endl;

        return false;
    }

    /*
        ============================================================
        BUNDLED PREFIX
        ============================================================

        The bundled prefix lives beside RetroDisc:

            ResidentEvil/
                RetroDisc
                pfx
                gamedata

        It is copied ONLY when the persistent game prefix is missing.
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

        /*
            Verify again.
        */

        if(!prefixLooksValid(
            localPrefix
        ))
        {
            std::cerr
                << "Copied Wine prefix failed validation:"
                << std::endl
                << "    "
                << localPrefix
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

    /*
        Prefix overlay no longer exists.
    */

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
