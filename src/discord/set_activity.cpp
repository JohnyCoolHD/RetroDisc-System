#include "discord_internal.hpp"
#include "discord.hpp"

#include <chrono>
#include <iostream>
#include <string>
#include <unistd.h>


using namespace discord_internal;


/*
    ================================================================
    SET ACTIVITY
    ================================================================
*/

bool DiscordPresence::setActivity(
    const std::string& gameName,
    const std::string& assetKey
)
{
    if(!connected)
    {
        return false;
    }

    const auto now =
        std::chrono::duration_cast<
            std::chrono::seconds
        >(
            std::chrono::system_clock::now()
                .time_since_epoch()
        ).count();

    const std::string payload =
        "{"
            "\"cmd\":\"SET_ACTIVITY\","
            "\"args\":{"
                "\"pid\":" +
                std::to_string(
                    static_cast<long long>(
                        ::getpid()
                    )
                ) +
                ","
                "\"activity\":{"
                    "\"name\":\"RetroDisc\","
                    "\"details\":\"Playing " +
                    jsonEscape(
                        gameName
                    ) +
                    "\","
                    "\"timestamps\":{"
                        "\"start\":" +
                        std::to_string(
                            static_cast<long long>(
                                now
                            )
                        ) +
                    "},"
                    "\"assets\":{"
                        "\"large_image\":\"" +
                        jsonEscape(
                            assetKey
                        ) +
                        "\","
                        "\"large_text\":\"" +
                        jsonEscape(
                            gameName
                        ) +
                        "\""
                    "}"
                "}"
            "},"
            "\"nonce\":\"retrodisc-set-activity\""
        "}";

    if(
        !sendPacket(
            OP_FRAME,
            payload
        )
    )
    {
        std::cerr
            << "Could not send Discord activity."
            << std::endl;

        disconnect();

        return false;
    }

    /*
        Discord sends a response to SET_ACTIVITY.
        Consume it so the IPC stream stays synchronized.
    */

    std::string response;

    if(
        !receivePacket(
            response
        )
    )
    {
        std::cerr
            << "Discord activity response was not received."
            << std::endl;

        disconnect();

        return false;
    }

    std::cout
        << "Discord activity:"
        << std::endl
        << "    RetroDisc"
        << std::endl
        << "    "
        << gameName
        << std::endl
        << "    Asset: "
        << assetKey
        << std::endl;

    return true;
}
