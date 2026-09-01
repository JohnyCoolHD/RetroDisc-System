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
            ~/.RetroDisc/pfx

        Upperdir:
            <persistent game directory>/pfx

        Workdir:
            <persistent game directory>/.prefix_work

        Mountpoint:
            /tmp/RetroDisc-<gameId>/merged_prefix

        Wine uses the merged directory as its prefix.
    */

    std::filesystem::path globalPrefixDirectory;
    std::filesystem::path prefixLowerDirectory;
    std::filesystem::path prefixOverlayDirectory;
    std::filesystem::path prefixMergedDirectory;
    std::filesystem::path prefixWorkDirectory;

    bool prefixOverlayMounted = false;



    /*
        ============================================================
        DISCORD
        ============================================================
    */

    DiscordPresence discord;

    bool discordEnabled = false;
};