#include "prefix_sanitize_internal.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <ios>
#include <system_error>


namespace prefix_sanitize_internal
{


bool filesEqual(
    const std::filesystem::path& first,
    const std::filesystem::path& second
)
{
    std::error_code ec;

    const auto firstSize =
        std::filesystem::file_size(
            first,
            ec
        );

    if(ec)
        return false;

    const auto secondSize =
        std::filesystem::file_size(
            second,
            ec
        );

    if(ec)
        return false;

    if(firstSize != secondSize)
        return false;

    std::ifstream firstFile(
        first,
        std::ios::binary
    );

    std::ifstream secondFile(
        second,
        std::ios::binary
    );

    if(
        !firstFile ||
        !secondFile
    )
    {
        return false;
    }

    static constexpr std::size_t bufferSize =
        1024 * 1024;

    std::array<char, bufferSize> firstBuffer{};
    std::array<char, bufferSize> secondBuffer{};

    while(firstFile && secondFile)
    {
        firstFile.read(
            firstBuffer.data(),
            firstBuffer.size()
        );

        secondFile.read(
            secondBuffer.data(),
            secondBuffer.size()
        );

        const auto firstCount =
            firstFile.gcount();

        const auto secondCount =
            secondFile.gcount();

        if(firstCount != secondCount)
            return false;

        if(
            !std::equal(
                firstBuffer.begin(),
                firstBuffer.begin() + firstCount,
                secondBuffer.begin()
            )
        )
        {
            return false;
        }
    }

    return true;
}


} // namespace prefix_sanitize_internal
