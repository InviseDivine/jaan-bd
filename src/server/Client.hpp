#pragma once
#include <string>
#define NOGDI
#define NOUSER
#include <enet.h>
#include <Types.hpp>

class Client {
private:
    std::string m_username;
    ENetPeer* m_peer;

    PlayerState m_state;

public:
    Client(ENetPeer* peer) : m_peer(peer) {}

    void packetReceived(ENetPacket* packet);
    bool sendPacket(char* data, int size);

    std::string getUsername() { return m_username; }

    PlayerState getPlayerState() { return m_state; }
    void loadPlayerState(PlayerState& state) { m_state = state; }

    int getId() { return m_peer->connectID; }
};