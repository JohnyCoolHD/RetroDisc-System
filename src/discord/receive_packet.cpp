#include "discord_internal.hpp"
#include "discord.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <unistd.h>


using namespace discord_internal;


/*
    ================================================================
    RECEIVE IPC PACKET
    ================================================================
*/

bool DiscordPresence::receivePacket(
    std::string& payload
)
{
    if(socketFd < 0)
    {
        return false;
    }

    std::uint32_t header[2];

    std::size_t headerOffset = 0;

    while(
        headerOffset <
        sizeof(header)
    )
    {
        const ssize_t received =
            ::read(
                socketFd,
                reinterpret_cast<char*>(
                    header
                ) + headerOffset,
                sizeof(header) - headerOffset
            );

        if(received <= 0)
        {
            return false;
        }

        headerOffset +=
            static_cast<std::size_t>(
                received
            );
    }

    const std::uint32_t opcode =
        header[0];

    const std::uint32_t length =
        header[1];

    (void)opcode;

    if(
        length >
        MAX_PACKET_SIZE
    )
    {
        return false;
    }

    payload.resize(
        length
    );

    std::size_t payloadOffset = 0;

    while(
        payloadOffset <
        length
    )
    {
        const ssize_t received =
            ::read(
                socketFd,
                payload.data() +
                    payloadOffset,
                length -
                    payloadOffset
            );

        if(received <= 0)
        {
            payload.clear();

            return false;
        }

        payloadOffset +=
            static_cast<std::size_t>(
                received
            );
    }

    return true;
}
