#pragma once
#include <Entity.hpp>

class Player : public Entity {
private:
    Vector2 m_speed;

    int m_animationTicks = 0;

public:
    Player(Texture2D& texture) : Entity(texture) {}
    
    Vector2 getPosition() { return Vector2 {m_bounds.x, m_bounds.y}; }

    void onTick();
    void draw();
};