#pragma once

#include "filesystem.hpp"


/*
============================================================
PROTON
============================================================
*/

std::filesystem::path resolveProton(
    const Context& ctx
);


/*
============================================================
GAME LAUNCH
============================================================

The public launch function is intentionally called:

    launchGame()

*/

bool launchGame(
    Context& ctx
);