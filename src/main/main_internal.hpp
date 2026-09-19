#pragma once

#include <string>

#include "context.hpp"


namespace main_internal
{


/*
    ================================================================
    DISCORD APPLICATION
    ================================================================
*/

constexpr const char* DISCORD_APPLICATION_ID =
    "1536835815546822716";


/*
    ================================================================
    LINUX PROCESS NAME
    ================================================================
*/

void setProcessName(
    const std::string& gameName
);


/*
    ================================================================
    COMMAND LINE
    ================================================================
*/

bool parseCommandLine(
    int argc,
    char* argv[],
    Context& ctx
);


} // namespace main_internal
