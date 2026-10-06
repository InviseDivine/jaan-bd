#include "Game.hpp"

#include <raylib.h>
#include <fstream>
#include <Player.hpp>
#include <Timer.hpp>
#include <algorithm>

#define RAYLIB_TMX_IMPLEMENTATION
#include "raylib-tmx.h"

#include <RemotePlayer.hpp>
#include <Multiplayer.hpp>


void Game::run() {
    InitWindow(1280, 720, "JaanBD");
    SetTargetFPS(60);

    tmx_map* map = LoadTMX("assets/testmap.tmx");
    Camera2D cam{
        .offset = Vector2{1280/2, 720/2},
        .target = Vector2{0.f, 0.f},
        .rotation = 0.f,
        .zoom = 5.f
    };

    TraceLog(LOG_INFO, "Map size in pixels %ix%i", map->tile_width * map->width, map->tile_height * map->height);

    std::string username;
    if(FileExists("username.txt")) {
        username = TextToLower(LoadFileText("username.txt"));
    } else {
        username = "jaan";
    }

    std::string texName = "assets/" + username + ".png";

    Texture2D plTex = LoadTexture(texName.c_str());

    if (!plTex.id) {
        TraceLog(LOG_FATAL, "Failed to load your skin");
    }

    auto& mult = Multiplayer::get();

    mult.init(username, "localhost", 7777);
    Player player(plTex, Vector2{(float)(map->tile_width * map->width), (float)(map->tile_height * map->height)});
    Timer timer(60.f);


    while(!WindowShouldClose()) {
        timer.advanceTime();

        for (int i = 0; i < timer.getTicks(); i++) {
            player.onTick();
        }

        Vector2 position = player.getPosition();
        cam.target = {
            std::clamp(position.x, cam.offset.x / cam.zoom, map->tile_width * (map->width - 1) - (cam.offset.x / cam.zoom)),
            std::clamp(position.y, cam.offset.y / cam.zoom, map->tile_height * (map->height - 1) - (cam.offset.y / cam.zoom))
        };

        mult.update();

        static std::vector<Entity*> players;

        players.clear();
        players.push_back(&player);

        for (auto& [id, remote] : m_remotes) {
            players.push_back(remote);
        }

        if (players.size() > 1) {
            std::sort(players.begin(), players.end(), [](Entity* a, Entity* b) {
                return a->getPosition().y < b->getPosition().y;
            });
        }

        BeginDrawing();
        ClearBackground(WHITE);
            BeginMode2D(cam);
                DrawTMX(map, 0, 0, WHITE, 1.0f);
                
                for (auto pl : players) pl->draw();
                
            EndMode2D();

            { // Connecting state
                static bool lastConnect = false;
                static float lastConnectTime = -10.f;

                bool shouldShowState = (GetTime() < lastConnectTime + 2.f);

                if (!mult.connected() && !shouldShowState) {
                    DrawText((mult.getReconnects() == 0) ? "Connecting..." : "Reconnecting...", 5, 695, 20, ColorAlpha(WHITE, fabsf(sinf(GetTime() * 5.f))));
                }

                if (lastConnect != mult.connected()) {
                    lastConnectTime = GetTime();
                }

                lastConnect = mult.connected();
                
                if (shouldShowState) {
                    DrawText((mult.connected()) ? "Connected!" : "Disconnected", 5, 695, 20, (mult.connected()) ? GREEN : RED);
                }
            }
        EndDrawing();
    }
}

bool Game::addRemotePlayer(AddRemotePlayerPacket& packet) {
    std::string username = packet.username;
    std::string filename = "assets/" + std::string(TextToLower(packet.username)) + ".png";

    TraceLog(LOG_INFO, "Loading skin %s", filename.c_str());

    Texture2D skin = LoadTexture(filename.c_str());

    if (skin.id == 0) return false;

    m_remotes[packet.id] = new RemotePlayer(skin, username, &packet.state);

    return true;
}

void Game::updateRemotePlayer(unsigned int id, PlayerState& state) {
    if (m_remotes.contains(id)) m_remotes[id]->updateState(state);
}

void Game::removeRemotePlayer(unsigned int id) {
    if (m_remotes.contains(id)) m_remotes.erase(id);
}