#pragma once
#include <unordered_map>
#include <Types.hpp>

class RemotePlayer;

class Game {
private:
    std::unordered_map<unsigned int, RemotePlayer*> m_remotes;

    Game() = default;
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
};