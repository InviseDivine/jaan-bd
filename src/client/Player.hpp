#pragma once
#include <Entity.hpp>

class Player : public Entity {
private:
    friend class Game;

    Vector2 m_speed;

    int m_animationTicks = 0;
    bool m_cake = false;

public:
    Player(Texture2D& texture) : Entity(texture) {}
    
    Vector2 getPosition() { return Vector2 {m_bounds.x, m_bounds.y}; }
    void resetSpeed() { m_speed = { 0 }; }

    void onTick();
    void draw();

    void tookCake() { m_cake = true; }
    void placeCake() { m_cake = false; }
};