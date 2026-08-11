#pragma once

#include "filesystem.hpp"


/*
============================================================
GAME LAUNCH
============================================================

The public launch function is intentionally called:

    launchGame()

NOT:

    LaunchWine()
*/

bool launchGame(Context& ctx);