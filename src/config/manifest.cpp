#include "config.hpp"

#include "context.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include <nlohmann/json.hpp>


using json = nlohmann::json;


/*
    ================================================================
    LOAD MANIFEST
    ================================================================
*/

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


        /*
            ========================================================
            GAME
            ========================================================
        */

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


        /*
            ========================================================
            RUNTIME
            ========================================================
        */

        ctx.runtime =
            data.value(
                "runtime",
                std::string("wine")
            );


        if(ctx.gameId.empty())
        {
            std::cerr
                << "Manifest game id is empty."
                << std::endl;

            return false;
        }


        if(ctx.gameName.empty())
        {
            std::cerr
                << "Manifest game name is empty."
                << std::endl;

            return false;
        }


        if(ctx.executable.empty())
        {
            std::cerr
                << "Manifest executable is empty."
                << std::endl;

            return false;
        }


        if(ctx.runtime.empty())
        {
            std::cerr
                << "Manifest runtime is empty."
                << std::endl;

            return false;
        }


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