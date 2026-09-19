#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>


namespace discord_internal
{


/*
    ================================================================
    DISCORD IPC
    ================================================================
*/

constexpr std::uint32_t OP_HANDSHAKE = 0;

constexpr std::uint32_t OP_FRAME = 1;

constexpr std::size_t MAX_PACKET_SIZE =
    1024 * 1024;


/*
    ================================================================
    SOCKET DIRECTORIES
    ================================================================
*/

std::vector<std::filesystem::path>
getSocketDirectories();


/*
    ================================================================
    JSON ESCAPING
    ================================================================
*/

std::string jsonEscape(
    const std::string& value
);


} // namespace discord_internal
