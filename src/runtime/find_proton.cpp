#include "runtime_internal.hpp"
#include "context.hpp"

#include <filesystem>
#include <iostream>
#include <system_error>
#include <vector>


namespace runtime_internal
{


/*
    ================================================================
    PROTON
    ================================================================
*/

std::filesystem::path findProton(
    const Context& ctx
)
{
    const auto home =
        getHome();

    if(home.empty())
    {
        return {};
    }


    /*
        ============================================================
        EXPLICIT PROTON PATH
        ============================================================

        Explicit configuration always has priority.

        Supported:

            "path":
                "/path/to/proton"

        or:

            "path":
                "/path/to/GE-Proton11-6-x86_64"

        In the second case RetroDisc expects:

            /path/to/GE-Proton11-6-x86_64/proton
    */

    if(!ctx.protonPath.empty())
    {
        const auto configured =
            std::filesystem::path(
                ctx.protonPath
            );

        std::error_code ec;

        if(
            std::filesystem::is_regular_file(
                configured,
                ec
            )
        )
        {
            return configured;
        }

        if(
            std::filesystem::is_directory(
                configured,
                ec
            )
        )
        {
            const auto proton =
                configured /
                "proton";

            if(
                std::filesystem::is_regular_file(
                    proton,
                    ec
                )
            )
            {
                return proton;
            }
        }

        std::cerr
            << "Configured Proton path is invalid:"
            << std::endl
            << "    "
            << configured
            << std::endl;
    }


    /*
        ============================================================
        PROTON VERSION
        ============================================================
    */

    if(ctx.protonVersion.empty())
    {
        return {};
    }


    /*
        ============================================================
        SEARCH ROOTS
        ============================================================

        Proton installations can come from:

            - Steam
            - ProtonUp-Qt
            - manual installations
            - other compatibility-tool managers

        RetroDisc therefore does not care where Proton came from.

        It searches known Steam compatibility-tool locations
        for the exact requested version.
    */

    const std::vector<std::filesystem::path> roots =
    {
        home / ".local" / "share" / "Steam",
        home / ".steam" / "root",
        home / ".steam" / "steam"
    };


    /*
        ============================================================
        COMPATIBILITYTOOLS.D
        ============================================================

        Examples:

            ~/.local/share/Steam/
                compatibilitytools.d/
                    GE-Proton11-6-x86_64/
                        proton

            ~/.steam/root/
                compatibilitytools.d/
                    GE-Proton11-6-x86_64/
                        proton

            ~/.steam/steam/
                compatibilitytools.d/
                    GE-Proton11-6-x86_64/
                        proton

        This is the important search path for ProtonUp-Qt and
        manually installed compatibility tools.
    */

    for(const auto& root : roots)
    {
        const auto proton =
            root /
            "compatibilitytools.d" /
            ctx.protonVersion /
            "proton";

        std::error_code ec;

        if(
            std::filesystem::is_regular_file(
                proton,
                ec
            )
        )
        {
            return proton;
        }
    }


    /*
        ============================================================
        STEAMAPPS/COMMON
        ============================================================

        Also support Proton installations that live directly
        inside Steam's normal compatibility-tool directory:

            ~/.local/share/Steam/
                steamapps/common/
                    Proton - Experimental/
                        proton
    */

    for(const auto& root : roots)
    {
        const auto proton =
            root /
            "steamapps" /
            "common" /
            ctx.protonVersion /
            "proton";

        std::error_code ec;

        if(
            std::filesystem::is_regular_file(
                proton,
                ec
            )
        )
        {
            return proton;
        }
    }


    /*
        ============================================================
        PROTON NOT FOUND
        ============================================================
    */

    std::cerr
        << "Proton version not found:"
        << std::endl
        << "    "
        << ctx.protonVersion
        << std::endl;

    std::cerr
        << "Searched:"
        << std::endl;

    for(const auto& root : roots)
    {
        std::cerr
            << "    "
            << (
                root /
                "compatibilitytools.d" /
                ctx.protonVersion /
                "proton"
            )
            << std::endl;
    }

    for(const auto& root : roots)
    {
        std::cerr
            << "    "
            << (
                root /
                "steamapps" /
                "common" /
                ctx.protonVersion /
                "proton"
            )
            << std::endl;
    }

    return {};
}


} // namespace runtime_internal
