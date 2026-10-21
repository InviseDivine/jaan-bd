#pragma once
#include <raylib.h>
#include <Types.hpp>

class Entity {
protected:
    Rectangle m_bounds;
    Texture2D m_tex;
    Direction m_direction = DOWN;

    int m_frame = 0;

public:
    Entity(Texture2D& texture) : m_tex(texture), m_bounds(Rectangle {0.f, 0.f, 12.f, 8.f}) {}
    
    Vector2 getPosition() { return Vector2 {m_bounds.x, m_bounds.y}; }
    Rectangle getBounds() const { return m_bounds; }

    void setPoisition(Vector2 position) { m_bounds.x = position.x; m_bounds.y = position.y; }

    virtual void onTick() = 0;
    virtual void draw() = 0;
};