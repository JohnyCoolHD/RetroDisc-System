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
        ~/.RetroDisc/prefix/<proton|wine>/<version>/

        A complete, reusable base prefix/compat-data tree, kept
        separately per runtime AND per Proton/Wine build/version.

    upperdir:
        ~/Games/RetroDisc/<gameId>/prefix

        or:

        <datapath>/prefix

        Contains only the actual changes made to the prefix
        (e.g. save files under AppData), since the base prefix
        already lives in the lowerdir.

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