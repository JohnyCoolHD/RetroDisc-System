#include "config.hpp"

#include "config_internal.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>


using json = nlohmann::json;


/*
    ================================================================
    LOAD CONFIG
    ================================================================
*/

bool loadConfig(
    Context& ctx
)
{
    const auto home =
        getHome();


    if(home.empty())
    {
        std::cerr
            << "HOME environment variable is not set."
            << std::endl;

        return false;
    }


    const auto configDirectory =
        getGameConfigDirectory(ctx);

    const auto path =
        getGameConfigPath(ctx);


    if(
        configDirectory.empty() ||
        path.empty()
    )
    {
        std::cerr
            << "Could not determine config path."
            << std::endl;

        return false;
    }


    /*
        ============================================================
        CREATE CONFIG IF MISSING
        ============================================================
    */

    std::error_code ec;

    if(
        !std::filesystem::is_regular_file(
            path,
            ec
        )
    )
    {
        std::cout
            << "Game config does not exist."
            << std::endl;


        if(
            !createDefaultGameConfig(
                ctx
            )
        )
        {
            std::cerr
                << "Failed to create default game config."
                << std::endl;

            return false;
        }
    }


    /*
        ============================================================
        OPEN CONFIG
        ============================================================
    */

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


            if(
                proton.contains("version") &&
                proton["version"].is_string()
            )
            {
                ctx.protonVersion =
                    proton["version"]
                        .get<std::string>();
            }


            if(
                proton.contains("path") &&
                proton["path"].is_string()
            )
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


            /*
                WINDOWS VERSION
            */

            if(
                wine.contains("windowsVersion") &&
                wine["windowsVersion"].is_string()
            )
            {
                ctx.wine.windowsVersion =
                    wine["windowsVersion"]
                        .get<std::string>();
            }


            /*
                ====================================================
                GRAPHICS
                ====================================================
            */

            if(
                wine.contains("graphics") &&
                wine["graphics"].is_object()
            )
            {
                const auto& graphics =
                    wine["graphics"];


                if(
                    graphics.contains("renderer") &&
                    graphics["renderer"].is_string()
                )
                {
                    ctx.wine.graphics.renderer =
                        graphics["renderer"]
                            .get<std::string>();
                }


                if(
                    graphics.contains("videoMemory") &&
                    graphics["videoMemory"].is_number_integer()
                )
                {
                    ctx.wine.graphics.videoMemory =
                        graphics["videoMemory"]
                            .get<int>();
                }


                if(
                    graphics.contains("fullscreen") &&
                    graphics["fullscreen"].is_boolean()
                )
                {
                    ctx.wine.graphics.fullscreen =
                        graphics["fullscreen"]
                            .get<bool>();
                }


                if(
                    graphics.contains("strictDrawOrdering") &&
                    graphics["strictDrawOrdering"].is_boolean()
                )
                {
                    ctx.wine.graphics.strictDrawOrdering =
                        graphics["strictDrawOrdering"]
                            .get<bool>();
                }
            }


            /*
                ====================================================
                SYNC
                ====================================================
            */

            if(
                wine.contains("sync") &&
                wine["sync"].is_object()
            )
            {
                const auto& sync =
                    wine["sync"];


                if(
                    sync.contains("esync") &&
                    sync["esync"].is_boolean()
                )
                {
                    ctx.wine.sync.esync =
                        sync["esync"]
                            .get<bool>();
                }


                if(
                    sync.contains("fsync") &&
                    sync["fsync"].is_boolean()
                )
                {
                    ctx.wine.sync.fsync =
                        sync["fsync"]
                            .get<bool>();
                }


                if(
                    sync.contains("ntsync") &&
                    sync["ntsync"].is_boolean()
                )
                {
                    ctx.wine.sync.ntsync =
                        sync["ntsync"]
                            .get<bool>();
                }
            }


            /*
                ====================================================
                DISPLAY
                ====================================================
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


                    if(
                        window.contains("decorations") &&
                        window["decorations"].is_boolean()
                    )
                    {
                        ctx.wine.display.window.decorations =
                            window["decorations"]
                                .get<bool>();
                    }


                    if(
                        window.contains("managed") &&
                        window["managed"].is_boolean()
                    )
                    {
                        ctx.wine.display.window.managed =
                            window["managed"]
                                .get<bool>();
                    }


                    if(
                        window.contains("mouseCapture") &&
                        window["mouseCapture"].is_boolean()
                    )
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


                    if(
                        scaling.contains("enabled") &&
                        scaling["enabled"].is_boolean()
                    )
                    {
                        ctx.wine.display.scaling.enabled =
                            scaling["enabled"]
                                .get<bool>();
                    }


                    if(
                        scaling.contains("mode") &&
                        scaling["mode"].is_string()
                    )
                    {
                        ctx.wine.display.scaling.mode =
                            scaling["mode"]
                                .get<std::string>();
                    }


                    if(
                        scaling.contains("filter") &&
                        scaling["filter"].is_string()
                    )
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
                    display.contains("virtualDesktop") &&
                    display["virtualDesktop"].is_object()
                )
                {
                    const auto& desktop =
                        display["virtualDesktop"];


                    if(
                        desktop.contains("enabled") &&
                        desktop["enabled"].is_boolean()
                    )
                    {
                        ctx.wine.display.virtualDesktop.enabled =
                            desktop["enabled"]
                                .get<bool>();
                    }


                    if(
                        desktop.contains("width") &&
                        desktop["width"].is_number_integer()
                    )
                    {
                        ctx.wine.display.virtualDesktop.width =
                            desktop["width"]
                                .get<int>();
                    }


                    if(
                        desktop.contains("height") &&
                        desktop["height"].is_number_integer()
                    )
                    {
                        ctx.wine.display.virtualDesktop.height =
                            desktop["height"]
                                .get<int>();
                    }
                }


                /*
                    DPI
                */

                if(
                    display.contains("dpi") &&
                    display["dpi"].is_number_integer()
                )
                {
                    ctx.wine.display.dpi =
                        display["dpi"]
                            .get<int>();
                }
            }


            /*
                ====================================================
                DLL OVERRIDES
                ====================================================
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


    /*
        ============================================================
        OUTPUT
        ============================================================
    */

    std::cout
        << "Loaded config:"
        << std::endl
        << "    "
        << path
        << std::endl;


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