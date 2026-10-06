#pragma once
#include <Entity.hpp>
#include <Types.hpp>
#include <string>

class RemotePlayer : public Entity {
protected:
    std::string m_username;

public:
    RemotePlayer(Texture2D& tex, std::string username, PlayerState* state = nullptr) 
        : Entity(tex), m_username(username) { if (state != nullptr) updateState(*state); }

    void updateState(PlayerState& state);

    void onTick() override {}
    void draw();
};