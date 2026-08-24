#pragma once

#include <filesystem>

#include "context.hpp"


/*
    ================================================================
    GAME FILESYSTEM OVERLAY
    ================================================================

    lowerdir:
        Original game installation

    upperdir:
        Persistent per-game gamedata

    merged:
        Temporary mounted game directory

    workdir:
        Temporary fuse-overlayfs work directory
*/

bool prepareFilesystem(
    Context& ctx
);


bool mountOverlay(
    Context& ctx
);


/*
    Prefix overlays are intentionally disabled.

    The Wine/Proton prefix is now persistent directly at:

        ~/Games/RetroDisc/<gameId>/pfx

    or:

        <datapath>/pfx
*/

bool mountPrefixOverlay(
    Context& ctx
);


/*
    ================================================================
    CLEANUP
    ================================================================
*/

bool cleanupFilesystem(
    Context& ctx
);


bool cleanup(
    Context& ctx
);