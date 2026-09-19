#include "discord_internal.hpp"
#include "discord.hpp"

#include <cstring>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>


using namespace discord_internal;


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
