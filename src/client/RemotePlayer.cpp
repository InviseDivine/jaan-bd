#include <RemotePlayer.hpp>

void RemotePlayer::updateState(PlayerState& state) {
    m_bounds.x = state.x;
    m_bounds.y = state.y;
    m_direction = state.direction;
    m_frame = state.frame;
}

void RemotePlayer::draw() {
    int line = (m_direction > 2) ? 2 : m_direction;
    Rectangle source = {m_frame * 20.f, line * 32.f, 20, 32};

    if (m_direction == RIGHT) source.width *= -1.f;

    DrawTexturePro(m_tex, source, m_bounds, Vector2{m_bounds.width/2, m_bounds.height}, 0.f, WHITE);
}