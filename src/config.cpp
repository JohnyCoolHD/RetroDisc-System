#include "config.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

using json = nlohmann::json;


bool loadManifest(
    Context& ctx
)
{
    const auto path =
        ctx.root /
        "manifest.json";

    std::ifstream file(
        path
    );

    if(!file)
    {
        std::cerr
            << "Manifest not found:"
            << std::endl
            << "    "
            << path
            << std::endl;

        return false;
    }

    try
    {
        json data;

        file >>
            data;

        ctx.gameId =
            data.at("game")
                .at("id")
                .get<std::string>();

        ctx.gameName =
            data.at("game")
                .at("name")
                .get<std::string>();

        ctx.executable =
            data.at("game")
                .at("executable")
                .get<std::string>();

        ctx.runtime =
            data.value(
                "runtime",
                std::string("wine")
            );

        std::cout
            << "Game: "
            << ctx.gameName
            << std::endl;

        std::cout
            << "Runtime: "
            << ctx.runtime
            << std::endl;
    }
    catch(const std::exception& e)
    {
        std::cerr
            << "Manifest error:"
            << std::endl
            << "    "
            << e.what()
            << std::endl;

        return false;
    }

    return true;
}


bool loadConfig(
    Context& ctx
)
{
    const char* home =
        std::getenv("HOME");

    if(home == nullptr)
    {
        std::cerr
            << "HOME environment variable is not set."
            << std::endl;

        return false;
    }

    const auto path =
        std::filesystem::path(home) /
        ".config" /
        "RetroDisc" /
        "games" /
        ctx.gameId /
        "config.json";

    std::ifstream file(
        path
    );

    if(!file)
    {
        std::cerr
            << "Config not found:"
            << std::endl
            << "    "
            << path
            << std::endl;

        return false;
    }

    try
    {
        json data;

        file >>
            data;

        /*
            ========================================================
            RUNTIME
            ========================================================
        */

        ctx.runtime =
            data.value(
                "runtime",
                ctx.runtime
            );

        /*
            ========================================================
            LAUNCH
            ========================================================
        */

        if(
            data.contains("launch") &&
            data["launch"].is_object()
        )
        {
            const auto& launch =
                data["launch"];

            if(
                launch.contains("arguments") &&
                launch["arguments"].is_array()
            )
            {
                ctx.arguments =
                    launch["arguments"]
                        .get<
                            std::vector<std::string>
                        >();
            }
        }

        /*
            ========================================================
            ENVIRONMENT
            ========================================================
        */

        if(
            data.contains("environment") &&
            data["environment"].is_object()
        )
        {
            ctx.environment =
                data["environment"]
                    .get<
                        std::map<
                            std::string,
                            std::string
                        >
                    >();
        }

        /*
            ========================================================
            PROTON
            ========================================================
        */

        if(
            data.contains("proton") &&
            data["proton"].is_object()
        )
        {
            const auto& proton =
                data["proton"];

            if(proton.contains("version"))
            {
                ctx.protonVersion =
                    proton["version"]
                        .get<std::string>();
            }

            if(proton.contains("path"))
            {
                ctx.protonPath =
                    proton["path"]
                        .get<std::string>();
            }
        }

        /*
            ========================================================
            WINE
            ========================================================
        */

        if(
            data.contains("wine") &&
            data["wine"].is_object()
        )
        {
            const auto& wine =
                data["wine"];

            if(
                wine.contains(
                    "windowsVersion"
                )
            )
            {
                ctx.wine.windowsVersion =
                    wine["windowsVersion"]
                        .get<std::string>();
            }

            /*
                GRAPHICS
            */

            if(
                wine.contains("graphics") &&
                wine["graphics"].is_object()
            )
            {
                const auto& graphics =
                    wine["graphics"];

                if(graphics.contains(
                    "renderer"
                ))
                {
                    ctx.wine.graphics.renderer =
                        graphics["renderer"]
                            .get<std::string>();
                }

                if(graphics.contains(
                    "videoMemory"
                ))
                {
                    ctx.wine.graphics.videoMemory =
                        graphics["videoMemory"]
                            .get<int>();
                }

                if(graphics.contains(
                    "fullscreen"
                ))
                {
                    ctx.wine.graphics.fullscreen =
                        graphics["fullscreen"]
                            .get<bool>();
                }

                if(graphics.contains(
                    "strictDrawOrdering"
                ))
                {
                    ctx.wine.graphics.strictDrawOrdering =
                        graphics["strictDrawOrdering"]
                            .get<bool>();
                }
            }

            /*
                SYNC
            */

            if(
                wine.contains("sync") &&
                wine["sync"].is_object()
            )
            {
                const auto& sync =
                    wine["sync"];

                if(sync.contains("esync"))
                {
                    ctx.wine.sync.esync =
                        sync["esync"]
                            .get<bool>();
                }

                if(sync.contains("fsync"))
                {
                    ctx.wine.sync.fsync =
                        sync["fsync"]
                            .get<bool>();
                }

                if(sync.contains("ntsync"))
                {
                    ctx.wine.sync.ntsync =
                        sync["ntsync"]
                            .get<bool>();
                }
            }

            /*
                DISPLAY
            */

            if(
                wine.contains("display") &&
                wine["display"].is_object()
            )
            {
                const auto& display =
                    wine["display"];

                /*
                    WINDOW
                */

                if(
                    display.contains("window") &&
                    display["window"].is_object()
                )
                {
                    const auto& window =
                        display["window"];

                    if(window.contains(
                        "decorations"
                    ))
                    {
                        ctx.wine.display.window.decorations =
                            window["decorations"]
                                .get<bool>();
                    }

                    if(window.contains(
                        "managed"
                    ))
                    {
                        ctx.wine.display.window.managed =
                            window["managed"]
                                .get<bool>();
                    }

                    if(window.contains(
                        "mouseCapture"
                    ))
                    {
                        ctx.wine.display.window.mouseCapture =
                            window["mouseCapture"]
                                .get<bool>();
                    }
                }

                /*
                    SCALING
                */

                if(
                    display.contains("scaling") &&
                    display["scaling"].is_object()
                )
                {
                    const auto& scaling =
                        display["scaling"];

                    if(scaling.contains(
                        "enabled"
                    ))
                    {
                        ctx.wine.display.scaling.enabled =
                            scaling["enabled"]
                                .get<bool>();
                    }

                    if(scaling.contains(
                        "mode"
                    ))
                    {
                        ctx.wine.display.scaling.mode =
                            scaling["mode"]
                                .get<std::string>();
                    }

                    if(scaling.contains(
                        "filter"
                    ))
                    {
                        ctx.wine.display.scaling.filter =
                            scaling["filter"]
                                .get<std::string>();
                    }
                }

                /*
                    VIRTUAL DESKTOP
                */

                if(
                    display.contains(
                        "virtualDesktop"
                    ) &&
                    display["virtualDesktop"].is_object()
                )
                {
                    const auto& desktop =
                        display["virtualDesktop"];

                    if(desktop.contains(
                        "enabled"
                    ))
                    {
                        ctx.wine.display.virtualDesktop.enabled =
                            desktop["enabled"]
                                .get<bool>();
                    }

                    if(desktop.contains(
                        "width"
                    ))
                    {
                        ctx.wine.display.virtualDesktop.width =
                            desktop["width"]
                                .get<int>();
                    }

                    if(desktop.contains(
                        "height"
                    ))
                    {
                        ctx.wine.display.virtualDesktop.height =
                            desktop["height"]
                                .get<int>();
                    }
                }

                /*
                    DPI
                */

                if(display.contains(
                    "dpi"
                ))
                {
                    ctx.wine.display.dpi =
                        display["dpi"]
                            .get<int>();
                }
            }

            /*
                DLL OVERRIDES
            */

            if(
                wine.contains("dllOverrides") &&
                wine["dllOverrides"].is_object()
            )
            {
                ctx.wine.dllOverrides =
                    wine["dllOverrides"]
                        .get<
                            std::map<
                                std::string,
                                std::string
                            >
                        >();
            }
        }
    }
    catch(const std::exception& e)
    {
        std::cerr
            << "Config error:"
            << std::endl
            << "    "
            << e.what()
            << std::endl;

        return false;
    }

    std::cout
        << "Loaded runtime: "
        << ctx.runtime
        << std::endl;

    std::cout
        << "Virtual Desktop: "
        << (
            ctx.wine.display.virtualDesktop.enabled
                ? "enabled"
                : "disabled"
        )
        << std::endl;

    if(
        ctx.wine.display.virtualDesktop.enabled
    )
    {
        std::cout
            << "Virtual Desktop Size: "
            << ctx.wine.display.virtualDesktop.width
            << "x"
            << ctx.wine.display.virtualDesktop.height
            << std::endl;
    }

    return true;
}