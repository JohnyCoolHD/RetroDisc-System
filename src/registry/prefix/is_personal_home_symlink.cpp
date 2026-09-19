#include "prefix_internal.hpp"

#include <filesystem>
#include <system_error>


namespace prefix_internal
{


bool isPersonalHomeSymlink(
    const std::filesystem::path& source,
    const std::filesystem::path& linkTarget
)
{
    std::filesystem::path target = linkTarget;

    if(target.is_relative())
        target = source.parent_path() / target;

    std::error_code ec;

    const auto canonical =
        std::filesystem::weakly_canonical(
            target,
            ec
        );

    if(ec)
        return false;

    const auto normalized =
        canonical.lexically_normal().string();

    return
        normalized == "/home" ||
        normalized.rfind(
            "/home/",
            0
        ) == 0;
}


} // namespace prefix_internal
