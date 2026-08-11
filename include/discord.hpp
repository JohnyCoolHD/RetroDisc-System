#pragma once

#include <string>


class DiscordPresence
{
public:

    DiscordPresence();

    ~DiscordPresence();


    bool connect(
        const std::string& applicationId
    );


    bool setActivity(
        const std::string& gameName,
        const std::string& gameId
    );


    void clearActivity();


    void disconnect();


    bool isConnected() const;


private:

    int socketFd;

    bool connected;


    bool sendPacket(
        int opcode,
        const std::string& payload
    );


    bool receivePacket(
        std::string& payload
    );


    std::string findSocket() const;
};
