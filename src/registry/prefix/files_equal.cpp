#include "prefix_internal.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <ios>
#include <system_error>


namespace prefix_internal
{


/*
    ================================================================
    COPY BUNDLED PREFIX AS DELTA
    ================================================================

    The global Wine prefix is the immutable LOWER layer.

    The bundled game prefix is compared against that lower layer.
    Only differences are copied into the persistent UPPER.

        global prefix
            +
        bundled differences
            =
        merged prefix

    The persistent game prefix therefore remains an overlay delta.
*/


bool filesEqual(
    const std::filesystem::path& left,
    const std::filesystem::path& right
)
{
    std::error_code ec;

    const auto leftStatus =
        std::filesystem::symlink_status(left, ec);

    if(ec || !std::filesystem::is_regular_file(leftStatus))
        return false;

    ec.clear();

    const auto rightStatus =
        std::filesystem::symlink_status(right, ec);

    if(ec || !std::filesystem::is_regular_file(rightStatus))
        return false;

    ec.clear();

    const auto leftSize =
        std::filesystem::file_size(left, ec);

    if(ec)
        return false;

    ec.clear();

    const auto rightSize =
        std::filesystem::file_size(right, ec);

    if(ec || leftSize != rightSize)
        return false;

    ec.clear();

    if(std::filesystem::equivalent(left, right, ec) && !ec)
        return true;

    std::ifstream a(left, std::ios::binary);
    std::ifstream b(right, std::ios::binary);

    if(!a.is_open() || !b.is_open())
        return false;

    std::array<char, 64 * 1024> ab{};
    std::array<char, 64 * 1024> bb{};

    for(;;)
    {
        a.read(
            ab.data(),
            static_cast<std::streamsize>(ab.size())
        );

        b.read(
            bb.data(),
            static_cast<std::streamsize>(bb.size())
        );

        if(a.gcount() != b.gcount())
            return false;

        if(!std::equal(
            ab.begin(),
            ab.begin() + a.gcount(),
            bb.begin()
        ))
        {
            return false;
        }

        if(a.eof() && b.eof())
            return true;

        if(!a && !a.eof())
            return false;

        if(!b && !b.eof())
            return false;
    }
}


} // namespace prefix_internal
