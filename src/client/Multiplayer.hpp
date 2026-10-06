#pragma once
#include <string>
#define NOGDI
#define NOUSER
#include <enet.h>

class Multiplayer {
public:
    static inline Multiplayer& get() {
        static Multiplayer inst;
        return inst;
    }

    void init(std::string nickname, std::string ip, int port = 6829);
    void update();

    void handlePacket(ENetPacket* packet);
    void sendPacket(char* data, int size);

    void disconnect();

    int getReconnects() const { return m_reconnects; }
    bool connected() const { return m_connected; }
private:
    ENetHost* m_client;
    ENetPeer* m_peer;

    ENetAddress m_address;
    std::string m_username;

    bool m_connected = false;

    float m_timeout = 0.f;
    int m_reconnects = 0;
};