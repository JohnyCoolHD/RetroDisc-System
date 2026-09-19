#include "prefix_internal.hpp"
#include "initialization_lock.hpp"
#include "../registry_internal.hpp"
#include "context.hpp"
#include "runtime.hpp"
#include "../../filesystem/filesystem_internal.hpp"

#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <system_error>


namespace prefix_internal
{


/*
    ================================================================
    INITIALIZE GLOBAL PREFIX
    ================================================================
*/


bool initializeGlobalPrefix(
    Context& ctx
)
{
    const auto home =
        getHome();


    if(home.empty())
    {
        std::cerr
            << "Could not determine HOME directory."
            << std::endl;

        return false;
    }


    const std::string runtimeFolder =
        ctx.runtime.empty()
            ? std::string("wine")
            : ctx.runtime;

    const bool useProton =
        (runtimeFolder == "proton");


    std::filesystem::path proton;

    if(useProton)
    {
        proton =
            resolveProton(ctx);

        if(proton.empty())
        {
            std::cerr
                << "Could not find Proton for global base prefix."
                << std::endl;

            return false;
        }

        std::error_code canonicalError;

        const auto canonicalProton =
            std::filesystem::weakly_canonical(
                proton,
                canonicalError
            );

        ctx.resolvedProtonPath =
            canonicalError
                ? proton
                : canonicalProton;
    }


    const std::string versionIdentifier =
        useProton
            ? resolveProtonVersionIdentifier(
                ctx.resolvedProtonPath
            )
            : resolveWineVersionIdentifier();


    ctx.globalPrefixDirectory =
        resolveGlobalPrefixDirectory(
            home,
            runtimeFolder,
            versionIdentifier
        );


    const auto compatDataDirectory =
        ctx.globalPrefixDirectory;


    const auto lockPath =
        compatDataDirectory /
        ".init.lock";

    std::error_code ec;


    const bool globalPrefixExisted =
        std::filesystem::exists(
            ctx.globalPrefixDirectory,
            ec
        );

    if(ec)
    {
        std::cerr
            << "Could not inspect global base prefix:"
            << std::endl
            << "    "
            << ctx.globalPrefixDirectory
            << std::endl
            << "    "
            << ec.message()
            << std::endl;

        return false;
    }


    std::filesystem::create_directories(
        compatDataDirectory,
        ec
    );

    if(ec)
    {
        std::cerr
            << "Could not create global base prefix directory:"
            << std::endl
            << "    "
            << compatDataDirectory
            << std::endl
            << "    "
            << ec.message()
            << std::endl;

        return false;
    }


    InitializationLock lock(
        lockPath
    );

    if(!lock.isLocked())
    {
        std::cerr
            << "Could not lock global base prefix initialization:"
            << std::endl
            << "    "
            << lockPath
            << std::endl;

        return false;
    }


    if(globalPrefixExisted)
    {
        const auto prefixStatus =
            std::filesystem::symlink_status(
                ctx.globalPrefixDirectory,
                ec
            );

        if(ec)
        {
            std::cerr
                << "Could not inspect global base prefix:"
                << std::endl
                << "    "
                << ctx.globalPrefixDirectory
                << std::endl
                << "    "
                << ec.message()
                << std::endl;

            return false;
        }


        if(!std::filesystem::is_directory(
            prefixStatus
        ))
        {
            std::cerr
                << "Global base prefix path is not a real directory:"
                << std::endl
                << "    "
                << ctx.globalPrefixDirectory
                << std::endl;

            return false;
        }


        const bool prefixValid =
            useProton
                ? prefixLooksValid(
                    ctx.globalPrefixDirectory
                )
                : winePrefixLooksValid(
                    ctx.globalPrefixDirectory
                );


        if(!prefixValid)
        {
            std::cerr
                << "Global base prefix exists but is incomplete:"
                << std::endl
                << "    "
                << ctx.globalPrefixDirectory
                << std::endl;

            if(useProton)
            {
                std::cerr
                    << "Required Proton compat-data files:"
                    << std::endl
                    << "    "
                    << compatDataDirectory / "version"
                    << std::endl
                    << "    "
                    << compatDataDirectory / "tracked_files"
                    << std::endl;
            }
            else
            {
                std::cerr
                    << "Required Wine prefix files:"
                    << std::endl
                    << "    "
                    << ctx.globalPrefixDirectory / "pfx" / "drive_c"
                    << std::endl
                    << "    "
                    << ctx.globalPrefixDirectory / "pfx" / "dosdevices"
                    << std::endl
                    << "    "
                    << ctx.globalPrefixDirectory / "pfx" / "system.reg"
                    << std::endl
                    << "    "
                    << ctx.globalPrefixDirectory / "pfx" / "user.reg"
                    << std::endl;
            }

            std::cerr
                << "Refusing to overwrite the existing base prefix."
                << std::endl;

            return false;
        }


        if(useProton)
        {
            const auto versionFile =
                compatDataDirectory /
                "version";

            const auto trackedFiles =
                compatDataDirectory /
                "tracked_files";


            const auto versionStatus =
                std::filesystem::symlink_status(
                    versionFile,
                    ec
                );

            if(
                ec ||
                !std::filesystem::is_regular_file(
                    versionStatus
                )
            )
            {
                std::cerr
                    << "Global Proton compat-data version file is invalid:"
                    << std::endl
                    << "    "
                    << versionFile
                    << std::endl;

                return false;
            }


            ec.clear();


            const auto trackedStatus =
                std::filesystem::symlink_status(
                    trackedFiles,
                    ec
                );

            if(
                ec ||
                !std::filesystem::is_regular_file(
                    trackedStatus
                )
            )
            {
                std::cerr
                    << "Global Proton compat-data tracked_files is invalid:"
                    << std::endl
                    << "    "
                    << trackedFiles
                    << std::endl;

                return false;
            }
        }


        std::cout
            << "Global base prefix found ("
            << runtimeFolder
            << " / "
            << versionIdentifier
            << "):"
            << std::endl
            << "    "
            << ctx.globalPrefixDirectory
            << std::endl;

        return true;
    }


    std::filesystem::create_directories(
        compatDataDirectory,
        ec
    );

    if(ec)
    {
        std::cerr
            << "Could not create global base prefix:"
            << std::endl
            << "    "
            << ctx.globalPrefixDirectory
            << std::endl
            << "    "
            << ec.message()
            << std::endl;

        return false;
    }


    std::cout
        << "Initializing global base prefix ("
        << runtimeFolder
        << " / "
        << versionIdentifier
        << "):"
        << std::endl
        << "    "
        << ctx.globalPrefixDirectory
        << std::endl;


    if(useProton)
    {
        const auto steamInstallPath =
            findSteamRootForProton(
                ctx.resolvedProtonPath
            );


        if(steamInstallPath.empty())
        {
            std::cerr
                << "Could not determine Steam root for Proton:"
                << std::endl
                << "    "
                << ctx.resolvedProtonPath
                << std::endl;

            return false;
        }


        std::cout
            << "Steam install path:"
            << std::endl
            << "    "
            << steamInstallPath
            << std::endl;


        std::ostringstream command;


        command
            << "STEAM_COMPAT_CLIENT_INSTALL_PATH="
            << shellQuote(
                steamInstallPath.string()
            )
            << " "
            << "STEAM_COMPAT_DATA_PATH="
            << shellQuote(
                compatDataDirectory.string()
            )
            << " "
            << "WINEPREFIX="
            << shellQuote(
                (compatDataDirectory / "pfx").string()
            )
            << " "
            << shellQuote(
                ctx.resolvedProtonPath.string()
            )
            << " run "
            << shellQuote(
                "wineboot"
            );


        if(!runCommand(
            command.str(),
            true
        ))
        {
            std::cerr
                << "Could not initialize global base prefix."
                << std::endl;

            return false;
        }
    }
    else
    {
        std::ostringstream command;


        command
            << "WINEPREFIX="
            << shellQuote(
                (
                    ctx.globalPrefixDirectory /
                    "pfx"
                ).string()
            )
            << " wineboot";


        if(!runCommand(
            command.str(),
            true
        ))
        {
            std::cerr
                << "Could not initialize global base prefix."
                << std::endl;

            return false;
        }
    }


    {
        const std::filesystem::path winePrefix =
            ctx.globalPrefixDirectory /
            "pfx";


        const std::string waitCommand =
            "WINEPREFIX=" +
            shellQuote(
                winePrefix.string()
            ) +
            " wineserver -w";


        if(!runCommand(
            waitCommand,
            true
        ))
        {
            std::cerr
                << "Could not wait for Wine server after prefix initialization."
                << std::endl;

            return false;
        }
    }


    const bool prefixValid =
        useProton
            ? prefixLooksValid(
                ctx.globalPrefixDirectory
            )
            : winePrefixLooksValid(
                ctx.globalPrefixDirectory
            );


    if(!prefixValid)
    {
        std::cerr
            << "Wineboot completed, but the global base prefix is"
            << " incomplete:"
            << std::endl
            << "    "
            << ctx.globalPrefixDirectory
            << std::endl;

        return false;
    }


    if(useProton)
    {
        const auto versionFile =
            compatDataDirectory /
            "version";

        const auto trackedFiles =
            compatDataDirectory /
            "tracked_files";


        ec.clear();


        if(
            !std::filesystem::is_regular_file(
                versionFile,
                ec
            )
        )
        {
            std::cerr
                << "Proton did not create a valid version file:"
                << std::endl
                << "    "
                << versionFile
                << std::endl;

            return false;
        }


        ec.clear();


        if(
            !std::filesystem::is_regular_file(
                trackedFiles,
                ec
            )
        )
        {
            std::cerr
                << "Proton did not create a valid tracked_files file:"
                << std::endl
                << "    "
                << trackedFiles
                << std::endl;

            return false;
        }
    }


    std::cout
        << "Global base prefix initialized successfully:"
        << std::endl
        << "    "
        << ctx.globalPrefixDirectory
        << std::endl;


    return true;
}


} // namespace prefix_internal
