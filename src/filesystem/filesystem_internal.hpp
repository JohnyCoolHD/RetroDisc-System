#pragma once

#include <filesystem>
#include <string>


/*
    ================================================================
    COMMAND
    ================================================================
*/

std::string shellQuote(
    const std::string& value
);


bool runCommand(
    const std::string& command,
    bool showCommand = false
);


/*
    ================================================================
    MOUNT
    ================================================================
*/

bool unmountPath(
    const std::filesystem::path& path
);


bool isMounted(
    const std::filesystem::path& path
);


/*
    ================================================================
    DIRECTORY
    ================================================================
*/

bool removeDirectory(
    const std::filesystem::path& path
);


bool replicateDirectoryStructure(
    const std::filesystem::path& source,
    const std::filesystem::path& destination
);

/*
    ================================================================
    PREFIX OVERLAY
    ================================================================
*/

struct Context;

bool mountPrefixOverlay(
    Context& ctx
);

/*
    ================================================================
    RUNTIME
    ================================================================
*/

std::filesystem::path findProton(
    const Context& ctx
);