#pragma once
#include <unordered_map>
#define NOGDI
#define NOUSER
#include <enet.h>
#include <string>
#include <Types.hpp>

class Client;

class Server {
private:
    Server() = default;

    std::unordered_map<enet_uint32, Client*> m_clients;
    std::unordered_map<std::string, PlayerState> m_savedPlayers;

public:
    Server(Server&&) = delete;

    static Server& get() {
        static Server srv;
        return srv;
    }

    void run();
    void broadcast(char* data, int len, Client* exclude = nullptr);

    void broadcastState(Client* cl);
    void mapChanged(Client* cl);
    void clientLogined(Client* cl);
};