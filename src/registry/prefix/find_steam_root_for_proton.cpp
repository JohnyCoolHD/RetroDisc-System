#include "prefix_internal.hpp"

#include <filesystem>
#include <system_error>
#include <vector>


namespace prefix_internal
{


std::filesystem::path findSteamRootForProton(
    const std::filesystem::path& proton
)
{
    const auto home = getHome();

    if(home.empty() || proton.empty())
    {
        return {};
    }

    const std::vector<std::filesystem::path> roots =
    {
        home / ".local" / "share" / "Steam",
        home / ".steam" / "root",
        home / ".steam" / "steam"
    };

    std::error_code protonError;

    const auto normalizedProton =
        std::filesystem::weakly_canonical(
            proton,
            protonError
        );

    if(protonError)
    {
        return {};
    }

    for(const auto& root : roots)
    {
        std::error_code ec;

        const auto normalizedRoot =
            std::filesystem::weakly_canonical(
                root,
                ec
            );

        if(ec)
        {
            continue;
        }

        const auto relative =
            normalizedProton.lexically_relative(
                normalizedRoot
            );

        if(
            !relative.empty() &&
            relative != "." &&
            relative.native().rfind("..", 0) != 0
        )
        {
            return normalizedRoot;
        }
    }

    return {};
}


} // namespace prefix_internal
