#include "runtime_internal.hpp"

#include <filesystem>
#include <system_error>
#include <vector>


namespace runtime_internal
{


/*
    ================================================================
    STEAM ROOT
    ================================================================
*/

std::filesystem::path findSteamRoot()
{
    const auto home =
        getHome();

    if(home.empty())
    {
        return {};
    }

    const std::vector<std::filesystem::path> roots =
    {
        home / ".local" / "share" / "Steam",
        home / ".steam" / "root",
        home / ".steam" / "steam"
    };

    for(const auto& root : roots)
    {
        std::error_code ec;

        if(
            std::filesystem::is_directory(
                root / "steamapps",
                ec
            )
        )
        {
            return root;
        }
    }

    return {};
}


} // namespace runtime_internal
