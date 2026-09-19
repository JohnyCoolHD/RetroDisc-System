#include "discord_internal.hpp"
#include "discord.hpp"


/*
    ================================================================
    CONSTRUCTOR
    ================================================================
*/

DiscordPresence::DiscordPresence()
    :
    socketFd(-1),
    connected(false)
{
}
