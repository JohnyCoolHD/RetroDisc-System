#include "discord_internal.hpp"
#include "discord.hpp"

#include <iostream>
#include <string>
#include <unistd.h>


using namespace discord_internal;


/*
    ================================================================
    CLEAR ACTIVITY
    ================================================================
*/

void DiscordPresence::clearActivity()
{
    if(!connected)
    {
        return;
    }

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
                "\"activity\":null"
            "},"
            "\"nonce\":\"retrodisc-clear-activity\""
        "}";

    if(
        !sendPacket(
            OP_FRAME,
            payload
        )
    )
    {
        std::cerr
            << "Could not clear Discord activity."
            << std::endl;

        disconnect();

        return;
    }

    /*
        Consume Discord's response.
    */

    std::string response;

    if(
        !receivePacket(
            response
        )
    )
    {
        std::cerr
            << "Discord clear-activity response was not received."
            << std::endl;

        disconnect();

        return;
    }

    std::cout
        << "Discord activity cleared."
        << std::endl;
}
