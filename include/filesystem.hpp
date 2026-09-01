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
    ================================================================
    WINE PREFIX OVERLAY
    ================================================================

    lowerdir:
        ~/.RetroDisc/pfx

    upperdir:
        ~/Games/RetroDisc/<gameId>/pfx

        or:

        <datapath>/pfx

    merged:
        /tmp/RetroDisc-<gameId>/merged_prefix

    workdir:
        <game directory>/.prefix_work

    The workdir must be on the same filesystem as the upperdir.
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