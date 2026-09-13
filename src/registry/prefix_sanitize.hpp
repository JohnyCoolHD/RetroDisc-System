#pragma once

#include <filesystem>

bool sanitizePersistentPrefixDirectory(
    const std::filesystem::path& prefixDirectory,
    const std::filesystem::path& lowerPrefixDirectory
);