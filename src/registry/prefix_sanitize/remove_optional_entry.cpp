#include "prefix_sanitize_internal.hpp"

#include <filesystem>
#include <iostream>
#include <system_error>


namespace prefix_sanitize_internal
{


bool removeOptionalEntry(
    const std::filesystem::path& path,
    const char* description
)
{
    std::error_code ec;

    const auto status =
        std::filesystem::symlink_status(
            path,
            ec
        );

    if(
        ec == std::errc::no_such_file_or_directory ||
        status.type() == std::filesystem::file_type::not_found
    )
    {
        return true;
    }

    if(ec)
    {
        std::cerr
            << "Failed to inspect persistent prefix "
            << description
            << ":"
            << std::endl
            << "    "
            << path
            << std::endl
            << "    "
            << ec.message()
            << std::endl;

        return false;
    }

    std::error_code removeEc;

    if(
        std::filesystem::is_directory(status) &&
        !std::filesystem::is_symlink(status)
    )
    {
        std::filesystem::remove_all(
            path,
            removeEc
        );
    }
    else
    {
        std::filesystem::remove(
            path,
            removeEc
        );
    }

    if(removeEc)
    {
        std::cerr
            << "Failed to remove persistent prefix "
            << description
            << ":"
            << std::endl
            << "    "
            << path
            << std::endl
            << "    "
            << removeEc.message()
            << std::endl;

        return false;
    }

    std::cout
        << "Removed persistent prefix "
        << description
        << ":"
        << std::endl
        << "    "
        << path
        << std::endl;

    return true;
}


} // namespace prefix_sanitize_internal
