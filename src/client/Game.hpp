#pragma once
#include <unordered_map>
#include <string>
#include <Types.hpp>
#include <raylib.h>
#include "tmx.h"
#include <vector>
#include <string>

class RemotePlayer;
class Entity;
class Player;

class Game {
private:
    std::unordered_map<unsigned int, RemotePlayer*> m_remotes;
    std::unordered_map<std::string, tmx_map*> m_maps;
    Player* m_player;

    tmx_map* m_map;

    Game() = default;

    std::vector<std::string> m_messages;
public:
    Game(Game&&) = delete;

    static Game& get() {
        static Game game;
        return game;
    }

    void run();

    bool addRemotePlayer(AddRemotePlayerPacket& packet);
    void updateRemotePlayer(unsigned int id, PlayerState& state);
    void removeRemotePlayer(unsigned int id);
    void removeAllRemotePlayers();

    // do not like this i just have no time
    float verifyCollisionX(Rectangle bounds, float speedX);
    float verifyCollisionY(Rectangle bounds, float speedY);

    void checkInteracts(Rectangle bounds);

    Vector2 findSpawn(tmx_map* map);


    bool loadMap(std::string const& name);
    tmx_map* findMap(std::string const& name);
    
    void changeMap(std::string const& name);
    tmx_map* currentMap() { return m_map; }
    

    void addMessage(std::string message) { m_messages.push_back(message); }
};