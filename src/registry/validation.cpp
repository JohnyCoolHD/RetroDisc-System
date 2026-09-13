#include "registry_internal.hpp"

#include <filesystem>
#include <system_error>


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
*/

bool prefixLooksValid(
    const std::filesystem::path& prefix
)
{
    if(!directoryExists(prefix))
    {
        return false;
    }


    const auto pfxDirectory =
        prefix /
        "pfx";

    const auto driveC =
        pfxDirectory /
        "drive_c";

    const auto dosDevices =
        pfxDirectory /
        "dosdevices";

    const auto systemReg =
        pfxDirectory /
        "system.reg";

    const auto userReg =
        pfxDirectory /
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

bool winePrefixLooksValid(
    const std::filesystem::path& prefix
)
{
    if(!directoryExists(prefix))
        return false;


    const auto pfxDirectory =
        prefix / "pfx";

    const auto driveC =
        pfxDirectory / "drive_c";

    const auto dosDevices =
        pfxDirectory / "dosdevices";

    const auto systemReg =
        pfxDirectory / "system.reg";

    const auto userReg =
        pfxDirectory / "user.reg";


    if(!directoryExists(driveC))
        return false;

    if(!directoryExists(dosDevices))
        return false;

    if(!regularFileExists(systemReg))
        return false;

    if(!regularFileExists(userReg))
        return false;


    return true;
}