#pragma once

#include <string>


/*
    ================================================================
    EMBEDDED RUNTIME CONFIGURATION
    ================================================================

    Runtime configuration files are embedded into the RetroDisc
    executable during the CMake build.

    The runtime name comes from manifest.json.

    Example:

        getEmbeddedRuntimeConfig(
            "proton",
            output
        );

    does NOT know about proton.json at compile time.

    CMake embeds every:

        runtime/*.json

    automatically.
*/


bool getEmbeddedRuntimeConfig(
    const std::string& runtime,
    std::string& output
);