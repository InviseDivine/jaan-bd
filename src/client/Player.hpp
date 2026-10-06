#pragma once
#include <Entity.hpp>

class Player : public Entity {
private:
    Vector2 m_speed;
    Vector2 m_mapSize;

    int m_animationTicks = 0;

public:
    Player(Texture2D& texture, Vector2 mapSize) : Entity(texture), m_mapSize(mapSize) {}
    
    Vector2 getPosition() { return Vector2 {m_bounds.x, m_bounds.y}; }

    void onTick();
    void draw();
};