#include "discord_internal.hpp"
#include "discord.hpp"


/*
    ================================================================
    STATUS
    ================================================================
*/

bool DiscordPresence::isConnected() const
{
    return connected;
}
