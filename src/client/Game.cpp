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

void Game::changeMap(std::string const& name, std::string spawnpoint = "spawnpoint") {
    auto map = findMap(name);

    m_map = map;
    m_player->setPoisition(findSpawn(map, spawnpoint));
    m_player->resetSpeed();

    auto& mult = Multiplayer::get();
    mult.mapChanged(name);
}

void Game::run() {
    InitWindow(1280, 720, "JaanBD");
    SetTargetFPS(60);

    std::string codepointsText = "АаБбВвГгДдЕеЁёЖжЗзИиЙйКкЛлМмНнОоПпРрСсТтУуФфХхЦцЧчШшЩщЪъЫыЬьЭэЮюЯяAaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz1234567890";
    int len = 0;
    int* codepoints = LoadCodepoints(codepointsText.c_str(), &len);

    m_marksman = LoadFontEx("assets/marksman.otf", 30, codepoints, len);
    SetTextureFilter(m_marksman.texture, TEXTURE_FILTER_POINT);
    UnloadCodepoints(codepoints);

    loadMap("untitled");
    loadMap("house");
    loadMap("kolyahhouse");
    loadMap("invdivhouse");
    loadMap("fullharmonyhouse");

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

        cam.target = position;

        // ----- VIBECODE AHTUNG!!! -----
        const float mapW = (float)(m_map->width  * m_map->tileWidth);
        const float mapH = (float)(m_map->height * m_map->tileHeight);

        const float left   = cam.offset.x / cam.zoom;
        const float right  = (GetScreenWidth()  - cam.offset.x) / cam.zoom;
        const float top    = cam.offset.y / cam.zoom;
        const float bottom = (GetScreenHeight() - cam.offset.y) / cam.zoom;

        const float minX = left;
        const float maxX = mapW - right;
        const float minY = top;
        const float maxY = mapH - bottom;

        cam.target.x = (minX > maxX) ? mapW * 0.5f : std::clamp(cam.target.x, minX, maxX);
        cam.target.y = (minY > maxY) ? mapH * 0.5f : std::clamp(cam.target.y, minY, maxY);
        // -----
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

        AnimateTMX(m_map);

        BeginDrawing();
        ClearBackground(BLACK);
            BeginMode2D(cam);
                DrawTMX(m_map, &cam, nullptr, 0, 0, WHITE);
                
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

            if (m_showDialog) {
                if (m_currentPos < m_dialogMessage.length()) {
                    m_currentPos++;
                }

                float padding = 10.f;
                float width = GetScreenWidth() - padding * 2;
                float height = 200.f;
                int thick = 5;
                
                float x = padding;
                float y = GetScreenHeight() - height - padding;
                DrawRectangle(x + thick, y + thick, width - thick, height - thick, BLACK);
                DrawRectangleLinesEx({x, y, width, height}, thick, WHITE);

                DrawTextEx(m_marksman, TextSubtext(m_dialogMessage.c_str(), 0, m_currentPos), {x + thick + padding, y + thick + padding}, 30, 1.f, WHITE);
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
                for (int x = 0; x < collided.propertiesLength; x++) {
                    auto spawnName = collided.properties[x];

                    if (strcmp(spawnName.name, "spawnpoint") == 0) {
                        changeMap(prop.stringValue, spawnName.stringValue);
                        return true;
                    }
                }

                changeMap(prop.stringValue);
                return true;
            }

            if (strcmp(prop.name, "dialogue") == 0 && (IsKeyPressed(KEY_E) || IsKeyPressed(KEY_ENTER)) && !m_showDialog) {
                m_currentPos = 0;
                m_dialogMessage = prop.stringValue;
                m_showDialog = true;

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

Vector2 Game::findSpawn(TmxMap* map, std::string spawnpoint) {
    std::string objname = spawnpoint;

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