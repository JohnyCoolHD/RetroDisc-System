#include "filesystem_internal.hpp"

#include <filesystem>


/*
    ================================================================
    UNMOUNT
    ================================================================
*/

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


    if(runCommand(
        "fusermount3 -u " +
        quoted +
        " >/dev/null 2>&1"
    ))
    {
        return true;
    }


    if(runCommand(
        "fusermount -u " +
        quoted +
        " >/dev/null 2>&1"
    ))
    {
        return true;
    }


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


/*
    ================================================================
    MOUNT CHECK
    ================================================================
*/

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