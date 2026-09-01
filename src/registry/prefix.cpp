#include "registry_internal.hpp"
#include "registry.hpp"
#include "context.hpp"
#include "runtime.hpp"
#include "../filesystem/filesystem_internal.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>


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


    return std::filesystem::path(
        home
    );
}


/*
    ================================================================
    COPY BUNDLED PREFIX WITHOUT WINDOWS
    ================================================================
*/


bool copyBundledPrefixWithoutWindows(
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
            << "Bundled Wine prefix source is not a directory:"
            << std::endl
            << "    "
            << source
            << std::endl;

        return false;
    }


    if(std::filesystem::exists(
        destination
    ))
    {
        std::cerr
            << "Wine prefix destination already exists:"
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


    const auto options =
        std::filesystem::directory_options::skip_permission_denied;


    std::filesystem::recursive_directory_iterator iterator(
        source,
        options,
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


    /*
        Never use std::filesystem::relative() here.

        Wine prefixes contain symbolic links and relative() may
        resolve them unexpectedly.
    */


    const std::string sourceRoot =
        source.lexically_normal().generic_string();


    const auto end =
        std::filesystem::recursive_directory_iterator();


    while(iterator != end)
    {
        const auto sourcePath =
            iterator->path();


        const std::string sourceString =
            sourcePath.generic_string();


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


        if(
            sourceString.compare(
                0,
                sourceRoot.size(),
                sourceRoot
            ) != 0
        )
        {
            std::cerr
                << "Wine prefix iterator escaped source directory:"
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


        const std::filesystem::path relative =
            std::filesystem::path(
                relativeString
            );


        /*
            ========================================================
            SKIP drive_c/windows
            ========================================================
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
            relative;


        /*
            ========================================================
            SYMLINKS
            ========================================================
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
                ----------------------------------------------------
                Skip personal /home links.
                ----------------------------------------------------
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
                ----------------------------------------------------
                Preserve Wine/system symlinks.
                ----------------------------------------------------
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
            ========================================================
            DIRECTORIES
            ========================================================
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
            ========================================================
            REGULAR FILES
            ========================================================
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
            ========================================================
            UNKNOWN ENTRY
            ========================================================
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


    ctx.globalPrefixDirectory =
        home /
        ".RetroDisc" /
        "pfx";


    std::error_code ec;


    /*
        ============================================================
        EXISTING GLOBAL PREFIX
        ============================================================
    */


    if(std::filesystem::exists(
        ctx.globalPrefixDirectory,
        ec
    ))
    {
        if(
            !std::filesystem::is_directory(
                ctx.globalPrefixDirectory,
                ec
            )
        )
        {
            std::cerr
                << "Global Wine prefix path is not a directory:"
                << std::endl
                << "    "
                << ctx.globalPrefixDirectory
                << std::endl;

            return false;
        }


        std::cout
            << "Global Wine prefix found:"
            << std::endl
            << "    "
            << ctx.globalPrefixDirectory
            << std::endl;

        return true;
    }


    /*
        ============================================================
        CREATE GLOBAL PREFIX
        ============================================================
    */


    std::filesystem::create_directories(
        ctx.globalPrefixDirectory,
        ec
    );


    if(ec)
    {
        std::cerr
            << "Could not create global Wine prefix:"
            << std::endl
            << "    "
            << ctx.globalPrefixDirectory
            << std::endl
            << "    "
            << ec.message()
            << std::endl;

        return false;
    }


    /*
        ============================================================
        FIND PROTON
        ============================================================
    */


    const auto proton =
        resolveProton(ctx);


    if(proton.empty())
    {
        std::cerr
            << "Could not find Proton for global Wine prefix."
            << std::endl;

        return false;
    }


    /*
        ============================================================
        STEAM INSTALL PATH
        ============================================================
    */


    const auto steamInstallPath =
        proton
            .parent_path()
            .parent_path()
            .parent_path()
            .parent_path();


    std::cout
        << "Initializing global Wine prefix with Proton:"
        << std::endl
        << "    "
        << ctx.globalPrefixDirectory
        << std::endl;


    std::cout
        << "Steam install path:"
        << std::endl
        << "    "
        << steamInstallPath
        << std::endl;


    /*
        ============================================================
        PROTON WINEBOOT
        ============================================================
    */


    std::ostringstream command;


    command
        << "STEAM_COMPAT_CLIENT_INSTALL_PATH="
        << shellQuote(
            steamInstallPath.string()
        )
        << " "
        << "STEAM_COMPAT_DATA_PATH="
        << shellQuote(
            ctx.globalPrefixDirectory.parent_path().string()
        )
        << " "
        << "WINEPREFIX="
        << shellQuote(
            ctx.globalPrefixDirectory.string()
        )
        << " "
        << shellQuote(
            proton.string()
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
            << "Could not initialize global Wine prefix."
            << std::endl;

        return false;
    }


    std::cout
        << "Global Wine prefix initialized successfully:"
        << std::endl
        << "    "
        << ctx.globalPrefixDirectory
        << std::endl;


    return true;
}


/*
    ================================================================
    PREPARE GAME PREFIX
    ================================================================
*/


bool prepareGamePrefix(
    Context& ctx
)
{
    /*
        ============================================================
        GLOBAL PREFIX
        ============================================================
    */


    if(!initializeGlobalPrefix(
        ctx
    ))
    {
        return false;
    }


    /*
        ============================================================
        GAME DIRECTORY
        ============================================================
    */


    if(ctx.gameDirectory.empty())
    {
        std::cerr
            << "Game directory is not configured."
            << std::endl;

        return false;
    }


    /*
        ============================================================
        PERSISTENT GAME DIRECTORY
        ============================================================
    */


    const auto persistentGameDirectory =
        ctx.dataPath.empty()
            ? getHome() /
              "Games" /
              "RetroDisc" /
              ctx.gameId
            : ctx.dataPath;


    /*
        ============================================================
        PREFIX PATHS
        ============================================================
    */


    ctx.globalPrefixDirectory =
        getHome() /
        ".RetroDisc" /
        "pfx";


    ctx.prefixLowerDirectory =
        ctx.globalPrefixDirectory;


    ctx.prefixOverlayDirectory =
        persistentGameDirectory /
        "pfx";


    ctx.prefixWorkDirectory =
        persistentGameDirectory /
        ".prefix_work";


    /*
        ============================================================
        TEMPORARY PREFIX DIRECTORY
        ============================================================
    */


    const auto temporaryPrefixDirectory =
        std::filesystem::path(
            "/tmp"
        ) /
        (
            std::string("RetroDisc-") +
            ctx.gameId
        );


    ctx.prefixMergedDirectory =
        temporaryPrefixDirectory /
        "merged_prefix";


    /*
        ============================================================
        CREATE TEMPORARY PREFIX DIRECTORY
        ============================================================
    */


    std::error_code ec;


    std::filesystem::create_directories(
        temporaryPrefixDirectory,
        ec
    );


    if(ec)
    {
        std::cerr
            << "Could not create temporary Proton directory:"
            << std::endl
            << "    "
            << temporaryPrefixDirectory
            << std::endl
            << "    "
            << ec.message()
            << std::endl;

        return false;
    }


    /*
        ============================================================
        PROTON pfx LINK
        ============================================================
    */


    const auto protonPrefixLink =
        temporaryPrefixDirectory /
        "pfx";


    ec.clear();


    if(
        std::filesystem::exists(
            protonPrefixLink,
            ec
        ) ||
        std::filesystem::is_symlink(
            protonPrefixLink,
            ec
        )
    )
    {
        ec.clear();


        std::filesystem::remove(
            protonPrefixLink,
            ec
        );


        if(ec)
        {
            std::cerr
                << "Could not remove old temporary Proton pfx link:"
                << std::endl
                << "    "
                << protonPrefixLink
                << std::endl
                << "    "
                << ec.message()
                << std::endl;

            return false;
        }
    }


    ec.clear();


    std::filesystem::create_symlink(
        "merged_prefix",
        protonPrefixLink,
        ec
    );


    if(ec)
    {
        std::cerr
            << "Could not create temporary Proton pfx link:"
            << std::endl
            << "    "
            << protonPrefixLink
            << " -> merged_prefix"
            << std::endl
            << "    "
            << ec.message()
            << std::endl;

        return false;
    }


    std::cout
        << "Temporary Proton pfx link created:"
        << std::endl
        << "    "
        << protonPrefixLink
        << " -> merged_prefix"
        << std::endl;


    /*
        ============================================================
        PROTON COMPATDATA METADATA
        ============================================================

        Proton expects tracked_files to exist during its first
        setup_prefix() call.

        Steam normally creates/maintains the CompatData directory
        before Proton is started. RetroDisc creates its own temporary
        CompatData directory, therefore we provide the initial empty
        tracked_files file here.

        Proton will populate this file itself during setup_prefix().
    */


    const auto trackedFiles =
        temporaryPrefixDirectory /
        "tracked_files";


    if(
        !std::filesystem::exists(
            trackedFiles,
            ec
        )
    )
    {
        ec.clear();


        std::ofstream trackedFile(
            trackedFiles
        );


        if(!trackedFile)
        {
            std::cerr
                << "Could not create Proton tracked_files:"
                << std::endl
                << "    "
                << trackedFiles
                << std::endl;

            return false;
        }


        trackedFile.close();


        if(!trackedFile)
        {
            std::cerr
                << "Could not finalize Proton tracked_files:"
                << std::endl
                << "    "
                << trackedFiles
                << std::endl;

            return false;
        }


        std::cout
            << "Temporary Proton tracked_files created:"
            << std::endl
            << "    "
            << trackedFiles
            << std::endl;
    }


    /*
        ============================================================
        PERSISTENT GAME PREFIX
        ============================================================
    */


    if(std::filesystem::exists(
        ctx.prefixOverlayDirectory
    ))
    {
        std::cout
            << "Persistent game prefix upper found:"
            << std::endl
            << "    "
            << ctx.prefixOverlayDirectory
            << std::endl;
    }
    else
    {
        /*
            ========================================================
            BUNDLED PREFIX
            ========================================================
        */


        const auto bundledPrefix =
            ctx.root /
            "pfx";


        if(std::filesystem::is_directory(
            bundledPrefix
        ))
        {
            std::cout
                << "Bundled game Wine prefix found:"
                << std::endl
                << "    "
                << bundledPrefix
                << std::endl;


            if(!copyBundledPrefixWithoutWindows(
                bundledPrefix,
                ctx.prefixOverlayDirectory
            ))
            {
                std::cerr
                    << "Could not copy bundled game Wine prefix:"
                    << std::endl
                    << "    "
                    << bundledPrefix
                    << std::endl
                    << "to:"
                    << std::endl
                    << "    "
                    << ctx.prefixOverlayDirectory
                    << std::endl;

                return false;
            }
        }
        else
        {
            /*
                ====================================================
                NO BUNDLED PREFIX
                ====================================================
            */


            std::filesystem::create_directories(
                ctx.prefixOverlayDirectory,
                ec
            );


            if(ec)
            {
                std::cerr
                    << "Could not create persistent game prefix upper:"
                    << std::endl
                    << "    "
                    << ctx.prefixOverlayDirectory
                    << std::endl
                    << "    "
                    << ec.message()
                    << std::endl;

                return false;
            }
        }
    }


    /*
        ============================================================
        MOUNT PREFIX OVERLAY
        ============================================================
    */


    std::cout
        << "Mounting prefix overlay..."
        << std::endl;


    if(!mountPrefixOverlay(
        ctx
    ))
    {
        std::cerr
            << "Could not mount Wine/Proton prefix overlay."
            << std::endl;

        return false;
    }


    /*
        ============================================================
        VERIFY MERGED PREFIX
        ============================================================
    */


    if(
        ctx.prefixMergedDirectory.empty() ||
        !std::filesystem::exists(
            ctx.prefixMergedDirectory
        )
    )
    {
        std::cerr
            << "Wine/Proton merged prefix was not created:"
            << std::endl
            << "    "
            << ctx.prefixMergedDirectory
            << std::endl;

        return false;
    }


    std::cout
        << "Wine/Proton merged prefix ready:"
        << std::endl
        << "    "
        << ctx.prefixMergedDirectory
        << std::endl;


    return true;
}

}


/*
    ================================================================
    PUBLIC PREFIX ENTRY POINT
    ================================================================
*/


bool preparePrefix(
    Context& ctx
)
{
    return prepareGamePrefix(
        ctx
    );
}
