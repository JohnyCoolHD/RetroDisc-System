#pragma once

#include <filesystem>


/*
    ================================================================
    VALIDATION
    ================================================================
*/

bool directoryExists(
    const std::filesystem::path& path
);


bool regularFileExists(
    const std::filesystem::path& path
);


bool prefixLooksValid(
    const std::filesystem::path& prefix
);


/*
    ================================================================
    COPY
    ================================================================
*/

bool copyPrefixEntry(
    const std::filesystem::path& source,
    const std::filesystem::path& destination
);


bool copyWinePrefix(
    const std::filesystem::path& source,
    const std::filesystem::path& destination
);