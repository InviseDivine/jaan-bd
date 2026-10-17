#pragma once
#include <unordered_map>
#include <string>
#include <Types.hpp>
#include <raylib.h>
#include <vector>
#include <string>
#include "raytmx.h"
class RemotePlayer;
class Entity;
class Player;

class Game {
private:
    std::unordered_map<unsigned int, RemotePlayer*> m_remotes;
    std::unordered_map<std::string, TmxMap*> m_maps;
    std::unordered_map<std::string, Sound> m_sounds;
    std::unordered_map<std::string, Music> m_music;
    Player* m_player;

    TmxMap* m_map;

    Game() = default;

    std::vector<std::string> m_messages;

    std::vector<std::string> m_dialogMessage;
    bool m_showDialog;
    int m_currentPos;
    int m_currentMessage;
    bool m_counterVisible = false;
    bool m_reconnectingVisible;

    Font m_marksman;
    Texture2D m_candle;
    int m_collectedCandles;

    std::string m_username;

    Music* m_currentTrack = nullptr;
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
    void verifyCollisionX();
    void verifyCollisionY();

    bool checkInteracts();

    Vector2 findSpawn(TmxMap* map, std::string spawnpoint = "spawnpoint");
    TmxLayer* findLayer(TmxMap* map, std::string const& name);

    bool loadMap(std::string const& name);
    TmxMap* findMap(std::string const& name);
    
    void changeMap(std::string const& name, std::string spawnpoint);
    TmxMap* currentMap() { return m_map; }

    TmxProperty* findProperty(TmxMap* map, std::string const& name);
    
    void addMessage(std::string message) { m_messages.push_back(message); }
};