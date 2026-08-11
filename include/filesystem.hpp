
#pragma once

#include <filesystem>

#include "context.hpp"

/*
    GAME FILESYSTEM OVERLAY

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

/*
    GAME OVERLAY
*/

bool mountOverlay(
    Context& ctx
);

/*
    PREFIX OVERLAY

    lowerdir:
        ~/.RetroDisc

    upperdir:
        ~/Games/RetroDisc/<gameId>/pfx

    merged:
        /tmp/RetroDiscPrefix_<pid>/merged

    workdir:
        /tmp/RetroDiscPrefixWork_<pid>/work
*/

bool mountPrefixOverlay(
    Context& ctx
);

/*
    CLEANUP
*/

bool cleanupFilesystem(
    Context& ctx
);

/*
    GENERAL CLEANUP
*/

bool cleanup(
    Context& ctx
);