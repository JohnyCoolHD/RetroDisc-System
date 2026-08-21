#include "filesystem.hpp"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <system_error>
#include <thread>

#include <sys/wait.h>
#include <unistd.h>


namespace
{


std::string shellQuote(
    const std::string& value
)
{
    std::string result = "'";

    for(const char c : value)
    {
        if(c == '\'')
        {
            result += "'\\''";
        }
        else
        {
            result += c;
        }
    }

    result += "'";

    return result;
}


bool runCommand(
    const std::string& command,
    bool showCommand = false
)
{
    if(showCommand)
    {
        std::cout
            << command
            << std::endl;
    }

    const int result =
        std::system(
            command.c_str()
        );

    if(result == -1)
    {
        return false;
    }

    if(!WIFEXITED(result))
    {
        return false;
    }

    return WEXITSTATUS(result) == 0;
}


bool unmountPath(
    const std::filesystem::path& path
)
{
    if(path.empty())
    {
        return true;
    }

    const std::string quoted =
        shellQuote(
            path.string()
        );

    /*
        Normal unmount.
    */

    if(runCommand(
        "fusermount3 -u " +
        quoted +
        " >/dev/null 2>&1"
    ))
    {
        return true;
    }

    /*
        Fallback.
    */

    if(runCommand(
        "fusermount -u " +
        quoted +
        " >/dev/null 2>&1"
    ))
    {
        return true;
    }

    /*
        Lazy unmount.
    */

    if(runCommand(
        "fusermount3 -uz " +
        quoted +
        " >/dev/null 2>&1"
    ))
    {
        return true;
    }

    if(runCommand(
        "fusermount -uz " +
        quoted +
        " >/dev/null 2>&1"
    ))
    {
        return true;
    }

    return false;
}


bool removeDirectory(
    const std::filesystem::path& path
)
{
    if(path.empty())
    {
        return true;
    }

    std::error_code ec;

    if(!std::filesystem::exists(
        path,
        ec
    ))
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


bool isMounted(
    const std::filesystem::path& path
)
{
    if(path.empty())
    {
        return false;
    }

    const std::string command =
        "findmnt -rn -M " +
        shellQuote(
            path.string()
        ) +
        " >/dev/null 2>&1";

    return runCommand(command);
}


/*
    ================================================================
    REPLICATE DIRECTORY STRUCTURE
    ================================================================
*/

bool replicateDirectoryStructure(
    const std::filesystem::path& source,
    const std::filesystem::path& destination
)
{
    std::error_code ec;

    if(!std::filesystem::exists(
        source,
        ec
    ))
    {
        std::cerr
            << "Source directory does not exist:"
            << std::endl
            << "    "
            << source
            << std::endl;

        return false;
    }

    if(!std::filesystem::is_directory(
        source,
        ec
    ))
    {
        std::cerr
            << "Source is not a directory:"
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

            if(!entry.is_directory(
                entryError
            ))
            {
                continue;
            }

            const auto relative =
                std::filesystem::relative(
                    entry.path(),
                    source,
                    ec
                );

            if(ec)
            {
                std::cerr
                    << "Could not determine relative directory:"
                    << std::endl
                    << "    "
                    << entry.path()
                    << std::endl
                    << "    "
                    << ec.message()
                    << std::endl;

                return false;
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


} // namespace


bool prepareFilesystem(
    Context& ctx
)
{
    if(ctx.executable.empty())
    {
        std::cerr
            << "Executable path in manifest is empty."
            << std::endl;

        return false;
    }


    /*
        ============================================================
        GAMEDATA
        ============================================================
    */

    ctx.gameDirectory =
        ctx.root /
        "gamedata";

    if(!std::filesystem::exists(
        ctx.gameDirectory
    ))
    {
        std::cerr
            << "GameData directory does not exist:"
            << std::endl
            << "    "
            << ctx.gameDirectory
            << std::endl;

        return false;
    }

    if(!std::filesystem::is_directory(
        ctx.gameDirectory
    ))
    {
        std::cerr
            << "GameData is not a directory:"
            << std::endl
            << "    "
            << ctx.gameDirectory
            << std::endl;

        return false;
    }


    /*
        ============================================================
        EXECUTABLE
        ============================================================
    */

    std::filesystem::path executablePath =
        ctx.executable;

    std::string executable =
        executablePath
            .lexically_normal()
            .generic_string();

    const std::string prefix =
        "gamedata/";

    if(executable.rfind(
        prefix,
        0
    ) == 0)
    {
        executable =
            executable.substr(
                prefix.size()
            );
    }

    while(
        !executable.empty() &&
        executable.front() == '/'
    )
    {
        executable.erase(
            executable.begin()
        );
    }

    ctx.executable =
        executable;


    /*
        ============================================================
        HOME
        ============================================================
    */

    const char* home =
        std::getenv("HOME");

    if(home == nullptr)
    {
        std::cerr
            << "HOME is not set."
            << std::endl;

        return false;
    }


    /*
        ============================================================
        PERSISTENT GAMEDATA
        ============================================================

        Without --datapath:

            ~/Games/RetroDisc/<gameId>/gamedata

        With --datapath:

            <datapath>/gamedata
    */

    std::filesystem::path persistentGameDirectory;

    if(!ctx.dataPath.empty())
    {
        persistentGameDirectory =
            ctx.dataPath;
    }
    else
    {
        persistentGameDirectory =
            std::filesystem::path(home) /
            "Games" /
            "RetroDisc" /
            ctx.gameId;
    }

    ctx.gameOverlayUpperDirectory =
        persistentGameDirectory /
        "gamedata";

    ctx.gameDataDirectory =
        ctx.gameOverlayUpperDirectory;


    /*
        ============================================================
        TEMPORARY OVERLAY
        ============================================================
    */

    const std::string pid =
        std::to_string(
            getpid()
        );

    ctx.mergedDirectory =
        std::filesystem::path("/tmp") /
        (
            "RetroDisc_" +
            pid
        );

    ctx.overlayWorkDirectory =
        std::filesystem::path("/tmp") /
        (
            "RetroDiscWork_" +
            pid
        ) /
        "work";


    try
    {
        std::filesystem::create_directories(
            ctx.gameOverlayUpperDirectory
        );

        std::filesystem::create_directories(
            ctx.mergedDirectory
        );

        std::filesystem::create_directories(
            ctx.overlayWorkDirectory
        );
    }
    catch(const std::exception& e)
    {
        std::cerr
            << "Could not create filesystem directories:"
            << std::endl
            << e.what()
            << std::endl;

        return false;
    }


    /*
        ============================================================
        DIRECTORY STRUCTURE
        ============================================================
    */

    std::cout
        << "Replicating GameData directory structure..."
        << std::endl;

    if(!replicateDirectoryStructure(
        ctx.gameDirectory,
        ctx.gameOverlayUpperDirectory
    ))
    {
        return false;
    }

    ctx.overlayMounted =
        false;

    ctx.prefixOverlayMounted =
        false;


    /*
        ============================================================
        STATUS
        ============================================================
    */

    std::cout
        << "GameData: "
        << ctx.gameDirectory
        << std::endl;

    std::cout
        << "Game overlay upper: "
        << ctx.gameOverlayUpperDirectory
        << std::endl;

    std::cout
        << "Temporary game mount: "
        << ctx.mergedDirectory
        << std::endl;

    std::cout
        << "Executable: "
        << ctx.executable
        << std::endl;


    return mountOverlay(ctx);
}


bool mountOverlay(
    Context& ctx
)
{
    if(ctx.overlayMounted)
    {
        return true;
    }

    if(
        ctx.gameDirectory.empty() ||
        ctx.gameOverlayUpperDirectory.empty() ||
        ctx.mergedDirectory.empty() ||
        ctx.overlayWorkDirectory.empty()
    )
    {
        std::cerr
            << "Game overlay paths are incomplete."
            << std::endl;

        return false;
    }

    std::error_code ec;

    if(!std::filesystem::is_directory(
        ctx.gameDirectory,
        ec
    ))
    {
        std::cerr
            << "GameData is not a directory:"
            << std::endl
            << "    "
            << ctx.gameDirectory
            << std::endl;

        return false;
    }

    if(!replicateDirectoryStructure(
        ctx.gameDirectory,
        ctx.gameOverlayUpperDirectory
    ))
    {
        return false;
    }

    if(isMounted(
        ctx.mergedDirectory
    ))
    {
        std::cerr
            << "Game overlay is already mounted:"
            << std::endl
            << "    "
            << ctx.mergedDirectory
            << std::endl;

        return false;
    }


    /*
        ============================================================
        FUSE OVERLAY
        ============================================================
    */

    const std::string command =
        "fuse-overlayfs "
        "-o lowerdir=" +
        shellQuote(
            ctx.gameDirectory.string()
        ) +
        " "
        "-o upperdir=" +
        shellQuote(
            ctx.gameOverlayUpperDirectory.string()
        ) +
        " "
        "-o workdir=" +
        shellQuote(
            ctx.overlayWorkDirectory.string()
        ) +
        " " +
        shellQuote(
            ctx.mergedDirectory.string()
        );

    std::cout
        << "Mounting game overlay..."
        << std::endl;

    if(!runCommand(
        command
    ))
    {
        std::cerr
            << "Game overlay mount failed."
            << std::endl;

        return false;
    }

    ctx.overlayMounted =
        true;


    /*
        ============================================================
        EXECUTABLE
        ============================================================
    */

    const auto executable =
        ctx.mergedDirectory /
        ctx.executable;

    if(!std::filesystem::exists(
        executable
    ))
    {
        std::cerr
            << "Executable is not visible through game overlay:"
            << std::endl
            << "    "
            << executable
            << std::endl;

        unmountPath(
            ctx.mergedDirectory
        );

        ctx.overlayMounted =
            false;

        return false;
    }

    std::cout
        << "Overlay executable: "
        << executable
        << std::endl;

    return true;
}


bool cleanupFilesystem(
    Context& ctx
)
{
    bool success =
        true;


    /*
        ============================================================
        GAME OVERLAY
        ============================================================
    */

    if(ctx.overlayMounted)
    {
        std::cout
            << "Unmounting game overlay..."
            << std::endl;

        bool unmounted =
            unmountPath(
                ctx.mergedDirectory
            );

        if(!unmounted)
        {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(
                    250
                )
            );

            unmounted =
                unmountPath(
                    ctx.mergedDirectory
                );
        }

        if(!unmounted)
        {
            std::cerr
                << "Could not unmount game overlay:"
                << std::endl
                << "    "
                << ctx.mergedDirectory
                << std::endl;

            success =
                false;
        }
        else
        {
            ctx.overlayMounted =
                false;
        }
    }


    /*
        ============================================================
        TEMPORARY DIRECTORIES
        ============================================================
    */

    if(!ctx.overlayMounted)
    {
        if(!removeDirectory(
            ctx.mergedDirectory
        ))
        {
            success =
                false;
        }

        if(!removeDirectory(
            ctx.overlayWorkDirectory.parent_path()
        ))
        {
            success =
                false;
        }
    }


    /*
        ============================================================
        PREFIX
        ============================================================
    */

    ctx.prefixOverlayMounted =
        false;

    return success;
}


bool cleanup(
    Context& ctx
)
{
    return cleanupFilesystem(ctx);
}