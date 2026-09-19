#include "discord_internal.hpp"
#include "discord.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <unistd.h>


using namespace discord_internal;


/*
    ================================================================
    SEND IPC PACKET
    ================================================================
*/

bool DiscordPresence::sendPacket(
    int opcode,
    const std::string& payload
)
{
    if(socketFd < 0)
    {
        return false;
    }

    if(
        payload.size() >
        MAX_PACKET_SIZE
    )
    {
        return false;
    }

    const std::uint32_t operation =
        static_cast<std::uint32_t>(
            opcode
        );

    const std::uint32_t length =
        static_cast<std::uint32_t>(
            payload.size()
        );

    std::uint8_t header[8];

    std::memcpy(
        header,
        &operation,
        sizeof(operation)
    );

    std::memcpy(
        header + 4,
        &length,
        sizeof(length)
    );

    std::size_t headerOffset = 0;

    while(
        headerOffset <
        sizeof(header)
    )
    {
        const ssize_t written =
            ::write(
                socketFd,
                header + headerOffset,
                sizeof(header) - headerOffset
            );

        if(written <= 0)
        {
            return false;
        }

        headerOffset +=
            static_cast<std::size_t>(
                written
            );
    }

    std::size_t payloadOffset = 0;

    while(
        payloadOffset <
        payload.size()
    )
    {
        const ssize_t written =
            ::write(
                socketFd,
                payload.data() +
                    payloadOffset,
                payload.size() -
                    payloadOffset
            );

        if(written <= 0)
        {
            return false;
        }

        payloadOffset +=
            static_cast<std::size_t>(
                written
            );
    }

    return true;
}
