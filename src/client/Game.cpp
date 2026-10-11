#include "Game.hpp"
#include <raylib.h>
#include <fstream>
#include <Player.hpp>
#include <Timer.hpp>
#include <algorithm>

#include <RemotePlayer.hpp>
#include <Multiplayer.hpp>

bool Game::loadMap(std::string const& name) {
    auto filename = "assets/" + name + ".tmx";
    auto map = LoadTMX(filename.c_str());

    if (!map) return false;

    m_maps[name] = map;
    return true;
}

TmxMap* Game::findMap(std::string const& name) {
    auto it = m_maps.find(name);

    if (it != m_maps.end()) {
        return it->second;
    }

    return nullptr;
}

void Game::changeMap(std::string const& name) {
    auto map = findMap(name);

    m_map = map;
    m_player->setPoisition(findSpawn(map));
    m_player->resetSpeed();

    auto& mult = Multiplayer::get();
    mult.mapChanged(name);
}

void Game::run() {
    InitWindow(1280, 720, "JaanBD");
    SetTargetFPS(60);

    loadMap("untitled");
    loadMap("house");

    m_map = findMap("house");

    Camera2D cam{
        .offset = Vector2{1280/2, 720/2},
        .target = Vector2{0.f, 0.f},
        .rotation = 0.f,
        .zoom = 5.f
    };

    TraceLog(LOG_INFO, "Map size in pixels %ix%i", m_map->tileWidth * m_map->width, m_map->tileHeight * m_map->height);

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
    m_player = new Player(plTex);
    Timer timer(60.f);

    if (m_map) {
        m_player->setPoisition(findSpawn(m_map)); // near bed
    }

    while(!WindowShouldClose()) {
        timer.advanceTime();

        for (int i = 0; i < timer.getTicks(); i++) {
            m_player->onTick();
        }

        Vector2 position = m_player->getPosition();
        Vector2 min = {
            cam.offset.x / cam.zoom,
            cam.offset.y / cam.zoom
        };

        Vector2 max = {
            m_map->tileWidth * (m_map->width - 1) - (cam.offset.x / cam.zoom),
            m_map->tileHeight * (m_map->height - 1) - (cam.offset.y / cam.zoom)
        };

        if (min.x < max.x && min.y < max.y) {
            cam.target = {
                std::clamp(position.x, min.x, max.x),
                std::clamp(position.y, min.x, max.y)
            };
        } else {    
            cam.target = {
                (float)m_map->tileWidth * (m_map->width - 1) / 2,
                (float)m_map->tileHeight * (m_map->height - 1) / 2
            };
        }

        mult.update();

        static std::vector<Entity*> players;

        { // update players array
            players.clear();
            players.push_back(m_player);

            for (auto& [id, remote] : m_remotes) {
                players.push_back(remote);
            }

            if (players.size() > 1) {
                std::sort(players.begin(), players.end(), [](Entity* a, Entity* b) {
                    return a->getPosition().y < b->getPosition().y;
                });
            }
        }


        BeginDrawing();
        ClearBackground(BLACK);
            BeginMode2D(cam);
                DrawTMX(m_map, &cam, nullptr, 0, 0, WHITE);
                
                static TmxObject collided;

                for (auto pl : players) pl->draw();
                DrawRectangleLinesEx(m_player->getBounds(), 0.3f, RED);
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

void Game::removeAllRemotePlayers() {
    m_remotes.clear();
}

bool Game::checkInteracts() {
    auto objectLayer = findLayer(m_map, "objects");

    TmxObject collided;

    if (objectLayer && CheckCollisionTMXObjectGroupRec(objectLayer->exact.objectGroup, m_player->m_bounds, &collided)) {        
        for (auto i = 0; i < collided.propertiesLength; i++) {
            auto prop = collided.properties[i];

            if (strcmp(prop.name, "transition") == 0) {
                changeMap(prop.stringValue);

                return true;
            }
        }
    }

    return false;
}

void Game::verifyCollisionX() {
    TmxObject collided;

    if (CheckCollisionTMXTileLayersRec(m_map, m_map->layers, m_map->layersLength, m_player->m_bounds, &collided)) {
        if (m_player->m_speed.x > 0)
            m_player->m_bounds.x = collided.aabb.x - m_player->m_bounds.width;
        else if (m_player->m_speed.x < 0)
            m_player->m_bounds.x = collided.aabb.x + collided.aabb.width;
    }
}

void Game::verifyCollisionY() {
    TmxObject collided;
    if (CheckCollisionTMXTileLayersRec(m_map, m_map->layers, m_map->layersLength, m_player->m_bounds, &collided)) {
        if (m_player->m_speed.y > 0)
            m_player->m_bounds.y = collided.aabb.y - m_player->m_bounds.height;
        else if (m_player->m_speed.y < 0)
            m_player->m_bounds.y = collided.aabb.y + collided.aabb.height;
    }
}

TmxLayer* Game::findLayer(TmxMap* map, std::string const& name) {
    for (int i = 0; i < map->layersLength; i++) {
        auto& layer = map->layers[i];

        if (name == layer.name) {
            return &map->layers[i];
        }
    }

    return nullptr;
}

Vector2 Game::findSpawn(TmxMap* map) {
    std::string objname = "spawnpoint";

    auto objectLayer = findLayer(map, "objects");

    if (!objectLayer) return Vector2();

    if (strcmp("objects", objectLayer->name) != 0) return Vector2();

    for (int i = 0; i < objectLayer->exact.objectGroup.objectsLength; i++) {
        auto obj = objectLayer->exact.objectGroup.objects[i];

        if (obj.name == objname) {
            return Vector2(obj.x, obj.y);
        }
    }

    return Vector2();
}