#include <Player.hpp>
#include <raylib.h>
#include <Types.hpp>
#include <Multiplayer.hpp>

#define ANIMATION_RATE 5
#define ANIMATION_FRAMES 4

void Player::onTick() {
    m_speed = { 0.f };

    if (IsKeyDown(KEY_W)) m_speed.y -= 1;
    if (IsKeyDown(KEY_S)) m_speed.y += 1;

    if (IsKeyDown(KEY_A)) m_speed.x -= 1;
    if (IsKeyDown(KEY_D)) m_speed.x += 1;

    m_bounds.x += m_speed.x * 0.7f;
    m_bounds.y += m_speed.y * 0.7f;

    if (m_speed.y > 0) m_direction = DOWN;
    if (m_speed.x > 0) m_direction = RIGHT;
    if (m_speed.x < 0) m_direction = LEFT;
    if (m_speed.y < 0) m_direction = UP;

    if (m_speed.x < -0.1f || m_speed.x > 0.1f || m_speed.y < -0.1f || m_speed.y > 0.1f) {
        m_animationTicks++;
    } else {
        m_frame = 0;
        m_animationTicks = 0;
    }

    m_speed.x *= 0.05f;
    m_speed.y *= 0.05f;


    if (m_animationTicks > ANIMATION_RATE) {
        m_frame++;
        m_animationTicks = 0; 
    }

    if (m_frame >= ANIMATION_FRAMES) {
        m_frame = 0;
    }

    if (m_bounds.x + m_bounds.width * 1.5f > m_mapSize.x) m_bounds.x = m_mapSize.x - m_bounds.width * 1.5f;
    if (m_bounds.y + m_bounds.height > m_mapSize.y) m_bounds.y = m_mapSize.y - m_bounds.height;
    if (m_bounds.x - m_bounds.width / 2 < 0.f) m_bounds.x = m_bounds.width / 2;
    if (m_bounds.y - m_bounds.height < 0.f) m_bounds.y = m_bounds.height;

    UpdatePlayerPacket packet = {
        .state = {
            .x = m_bounds.x, 
            .y = m_bounds.y,
            .direction = m_direction,
            .frame = m_frame
        }
    };

    auto& mult = Multiplayer::get();

    mult.sendPacket((char*)&packet, sizeof(packet));
}

void Player::draw() {
    int line = (m_direction > 2) ? 2 : m_direction;

    Rectangle source = {m_frame * 20.f, line * 32.f, 20, 32};

    if (m_direction == RIGHT) source.width *= -1.f;

    DrawTexturePro(m_tex, source, m_bounds, Vector2{m_bounds.width/2, m_bounds.height}, 0.f, WHITE);
}