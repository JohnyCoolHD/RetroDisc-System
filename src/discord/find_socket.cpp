#include "discord_internal.hpp"
#include "discord.hpp"

#include <filesystem>
#include <string>
#include <system_error>


using namespace discord_internal;


/*
    ================================================================
    FIND DISCORD IPC SOCKET
    ================================================================
*/

std::string
DiscordPresence::findSocket() const
{
    const auto directories =
        getSocketDirectories();

    for(
        const auto& directory :
        directories
    )
    {
        for(
            int index = 0;
            index < 10;
            ++index
        )
        {
            const auto socket =
                directory /
                (
                    "discord-ipc-" +
                    std::to_string(index)
                );

            std::error_code ec;

            if(
                std::filesystem::exists(
                    socket,
                    ec
                )
            )
            {
                return socket.string();
            }
        }
    }

    return {};
}
