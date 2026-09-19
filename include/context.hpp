#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include "discord.hpp"


struct WineGraphics
{
    std::string renderer;
    int videoMemory = 0;
    bool fullscreen = false;
    bool strictDrawOrdering = false;
};


struct WineSync
{
    bool esync = false;
    bool fsync = false;
    bool ntsync = false;
};


struct WineWindow
{
    bool decorations = true;
    bool managed = true;
    bool mouseCapture = true;
};


struct WineScaling
{
    bool enabled = false;
    std::string mode;
    std::string filter;
};


struct WineVirtualDesktop
{
    bool enabled = false;
    int width = 640;
    int height = 480;
};


struct WineDisplay
{
    WineWindow window;
    WineScaling scaling;
    WineVirtualDesktop virtualDesktop;
    int dpi = 96;
};


struct WineConfig
{
    std::string windowsVersion;

    WineGraphics graphics;
    WineSync sync;
    WineDisplay display;

    std::map<
        std::string,
        std::string
    > dllOverrides;
};


struct Context
{
    std::filesystem::path root;

    /*
        ============================================================
        PERSISTENT GAME DATA
        ============================================================

        Without --datapath:

            ~/Games/RetroDisc/<gameId>/

        With --datapath:

            <datapath>/

        The supplied datapath is the complete persistent directory.
    */

    std::filesystem::path dataPath;

    std::string gameId;
    std::string gameName;
    std::string executable;
    std::string runtime;

    std::vector<
        std::string
    > arguments;

    std::map<
        std::string,
        std::string
    > environment;

    std::string protonVersion;
    std::string protonPath;

    /* The exact Proton executable resolved for this launch. */
    std::filesystem::path resolvedProtonPath;

    WineConfig wine;


    /*
        ============================================================
        GAME FILESYSTEM
        ============================================================
    */

    std::filesystem::path gameDirectory;

    std::filesystem::path gameOverlayUpperDirectory;

    std::filesystem::path gameDataDirectory;

    std::filesystem::path mergedDirectory;

    std::filesystem::path overlayWorkDirectory;

    bool overlayMounted = false;



    /*
        ================================================================
        WINE PREFIX
        ================================================================

        The Wine prefix is assembled using fuse-overlayfs.

        Lowerdir:
            ~/.RetroDisc/prefix/<proton|wine>/<version>/

            The whole immutable base BUILD directory, kept separately
            per runtime AND per Proton/Wine build (e.g.
            "Proton - Experimental" vs "GE-Proton11-6-x86_64" are
            never mixed). For Proton this contains "version",
            "tracked_files" and "pfx/"; for Wine it contains only
            "pfx/". Using the whole build directory (rather than just
            its "pfx" subdirectory) as the lowerdir is deliberate: see
            prefixMergedPfxDirectory below.

        Persistent lower:
            <persistent game directory>/prefix

            Contains only persistent game-specific changes under
            prefix/pfx/.... Runtime-specific compat-data metadata is
            never persisted here.

        Runtime upper:
            /tmp/<gameId>-<pid>/prefix_upper

            fuse-overlayfs writes all copy-up activity here. It is
            discarded after the run; only real content changes are
            promoted to the persistent lower.

        Workdir:
            /tmp/<gameId>-<pid>/prefix_work

        Mountpoint:
            /tmp/<gameId>-<pid>/merged_prefix

        The actual, usable Wine prefix is the "pfx" subdirectory of
        that mountpoint -- see prefixMergedPfxDirectory.
    */

    std::filesystem::path globalPrefixDirectory;
    std::filesystem::path prefixLowerDirectory;
    std::filesystem::path prefixOverlayDirectory;
    std::filesystem::path prefixRuntimeUpperDirectory;
    std::filesystem::path prefixMergedDirectory;
    std::filesystem::path prefixWorkDirectory;

    /*
        ================================================================
        prefixMergedPfxDirectory = prefixMergedDirectory / "pfx"
        ================================================================

        This -- not prefixMergedDirectory itself -- is what
        WINEPREFIX is set to, and what every drive_c/... path is
        built from (see src/runtime/).

        For Proton, STEAM_COMPAT_DATA_PATH is set to
        prefixMergedDirectory (the whole merged tree, which also
        exposes "version" and "tracked_files" from the lowerdir).
        Because prefixLowerDirectory is the whole build directory,
        prefixMergedDirectory/"pfx" and prefixMergedDirectory ARE the
        same overlay mount as far as the filesystem is concerned --
        so it no longer matters whether Proton internally resolves
        its real prefix via WINEPREFIX or via
        $STEAM_COMPAT_DATA_PATH/pfx: both land in the same place, and
        both correctly copy up into <datapath>/prefix/pfx/...

        (An earlier approach kept STEAM_COMPAT_DATA_PATH as a
        separate, synthetic per-launch directory with a symlinked
        "pfx". That extra indirection made Proton treat compat-data
        as not properly initialized on every single launch, causing
        it to re-run a full prefix setup and write hundreds of MiB
        into <datapath>/prefix each time.)
    */

    std::filesystem::path prefixMergedPfxDirectory;

    bool prefixOverlayMounted = false;


    /*
        ================================================================
        PROTON COMPAT-DATA
        ================================================================

        There is no separate Context field for this anymore.

        Confirmed by actual testing: Proton does NOT reliably honor
        WINEPREFIX for its own real prefix I/O -- it derives the
        prefix it actually operates on from
        $STEAM_COMPAT_DATA_PATH/pfx. Rather than maintaining a
        separate, synthetic compat-data directory to work around
        that (an earlier approach, which made Proton re-run a full
        prefix setup on every launch), prefixLowerDirectory above is
        simply the whole build directory instead of just its "pfx"
        subdirectory. That makes prefixMergedDirectory itself a
        valid STEAM_COMPAT_DATA_PATH (see buildProtonCommand() in
        src/runtime/build_proton_command.cpp) -- no extra field needed.
    */


    /*
        ============================================================
        DISCORD
        ============================================================
    */

    DiscordPresence discord;

    bool discordEnabled = false;
};