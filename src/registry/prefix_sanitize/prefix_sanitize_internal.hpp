#pragma once

#include <filesystem>


namespace prefix_sanitize_internal
{


bool removeOptionalEntry(
    const std::filesystem::path& path,
    const char* description
);


bool filesEqual(
    const std::filesystem::path& first,
    const std::filesystem::path& second
);


bool removeRedundantWindowsFiles(
    const std::filesystem::path& upperWindowsDirectory,
    const std::filesystem::path& lowerWindowsDirectory
);


} // namespace prefix_sanitize_internal
