#include "discord_internal.hpp"
#include "discord.hpp"

#include <unistd.h>


/*
    ================================================================
    DISCONNECT
    ================================================================
*/

void DiscordPresence::disconnect()
{
    if(socketFd >= 0)
    {
        ::close(
            socketFd
        );
    }

    socketFd =
        -1;

    connected =
        false;
}
