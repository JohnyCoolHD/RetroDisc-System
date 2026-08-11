#include "discord.hpp"

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>


namespace
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
    DISCORD APPLICATION ID
    ================================================================

    Replace this with your Discord Application ID.

    This value is NOT a secret.

    Do NOT put a Discord bot token or client secret here.
*/

constexpr const char* DISCORD_APPLICATION_ID =
    "REPLACE_WITH_DISCORD_APPLICATION_ID";


/*
    ================================================================
    SOCKET DIRECTORIES
    ================================================================
*/

std::vector<std::filesystem::path>
getSocketDirectories()
{
    std::vector<std::filesystem::path>
        directories;

    const char* variables[] =
    {
        "XDG_RUNTIME_DIR",
        "TMPDIR",
        "TMP",
        "TEMP"
    };

    for(
        const char* variable :
        variables
    )
    {
        const char* value =
            std::getenv(variable);

        if(
            value != nullptr &&
            *value != '\0'
        )
        {
            directories.emplace_back(
                value
            );
        }
    }

    /*
        Always try /tmp as final fallback.
    */

    directories.emplace_back(
        "/tmp"
    );

    return directories;
}


/*
    ================================================================
    JSON ESCAPING
    ================================================================
*/

std::string jsonEscape(
    const std::string& value
)
{
    std::string result;

    result.reserve(
        value.size() + 16
    );

    for(
        const char c :
        value
    )
    {
        switch(c)
        {
            case '"':
                result += "\\\"";
                break;

            case '\\':
                result += "\\\\";
                break;

            case '\b':
                result += "\\b";
                break;

            case '\f':
                result += "\\f";
                break;

            case '\n':
                result += "\\n";
                break;

            case '\r':
                result += "\\r";
                break;

            case '\t':
                result += "\\t";
                break;

            default:

                /*
                    Ignore ASCII control characters.
                */

                if(
                    static_cast<unsigned char>(c)
                    < 0x20
                )
                {
                    continue;
                }

                result += c;
                break;
        }
    }

    return result;
}

} // namespace


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


/*
    ================================================================
    DESTRUCTOR
    ================================================================
*/

DiscordPresence::~DiscordPresence()
{
    disconnect();
}


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

    /*
        Write header completely.
    */

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

    /*
        Write payload completely.
    */

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

    const std::uint32_t length =
        header[1];

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


/*
    ================================================================
    CONNECT
    ================================================================
*/

bool DiscordPresence::connect(
    const std::string& applicationId
)
{
    if(applicationId.empty())
    {
        return false;
    }

    disconnect();

    const std::string socketPath =
        findSocket();

    if(socketPath.empty())
    {
        std::cerr
            << "Discord IPC socket not found."
            << std::endl;

        return false;
    }

    socketFd =
        ::socket(
            AF_UNIX,
            SOCK_STREAM,
            0
        );

    if(socketFd < 0)
    {
        std::cerr
            << "Could not create Discord IPC socket."
            << std::endl;

        return false;
    }

    sockaddr_un address{};

    address.sun_family =
        AF_UNIX;

    if(
        socketPath.size() >=
        sizeof(address.sun_path)
    )
    {
        std::cerr
            << "Discord IPC socket path is too long."
            << std::endl;

        disconnect();

        return false;
    }

    std::strncpy(
        address.sun_path,
        socketPath.c_str(),
        sizeof(address.sun_path) - 1
    );

    if(
        ::connect(
            socketFd,
            reinterpret_cast<sockaddr*>(
                &address
            ),
            sizeof(address)
        ) < 0
    )
    {
        std::cerr
            << "Could not connect to Discord IPC."
            << std::endl;

        disconnect();

        return false;
    }

    /*
        ============================================================
        HANDSHAKE
        ============================================================
    */

    const std::string handshake =
        "{"
            "\"v\":1,"
            "\"client_id\":\"" +
            jsonEscape(applicationId) +
            "\""
        "}";

    if(
        !sendPacket(
            OP_HANDSHAKE,
            handshake
        )
    )
    {
        std::cerr
            << "Discord handshake could not be sent."
            << std::endl;

        disconnect();

        return false;
    }

    std::string response;

    if(
        !receivePacket(
            response
        )
    )
    {
        std::cerr
            << "Discord handshake response was not received."
            << std::endl;

        disconnect();

        return false;
    }

    connected =
        true;

    std::cout
        << "Discord Rich Presence connected."
        << std::endl;

    return true;
}


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
    }
    else
    {
        std::cout
            << "Discord activity cleared."
            << std::endl;
    }
}


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


/*
    ================================================================
    STATUS
    ================================================================
*/

bool DiscordPresence::isConnected() const
{
    return connected;
}