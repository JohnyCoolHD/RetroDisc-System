#include "discord_internal.hpp"
#include "discord.hpp"


/*
    ================================================================
    DESTRUCTOR
    ================================================================
*/

DiscordPresence::~DiscordPresence()
{
    disconnect();
}
