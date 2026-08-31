#pragma once

#include <filesystem>

#include "context.hpp"


/*
    ================================================================
    CONFIGURATION PATHS
    ================================================================
*/

std::filesystem::path getHome();


std::filesystem::path getGameConfigDirectory(
    const Context& ctx
);


std::filesystem::path getGameConfigPath(
    const Context& ctx
);


/*
    ================================================================
    DEFAULT CONFIGURATION
    ================================================================
*/

bool createDefaultGameConfig(
    const Context& ctx
);