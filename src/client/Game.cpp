#include "Game.hpp"
#include <raylib.h>
#include <fstream>
#include <Player.hpp>
#include <Timer.hpp>
#include <algorithm>

#include <RemotePlayer.hpp>
#include <Multiplayer.hpp>

// void DrawCollisons(tmx_object *object, RaylibTMXCollision collision, void* userdata) {
//     // Vector2* position = (Vector2*)userdata;
//     switch (collision.type)
//     {
//         case COLLISION_RECT: {
//             Color color = object->obj_type == OT_TILE ? RED : BLUE;
//             // collision.rect.x += position->x;
//             // collision.rect.y += position->y;
//             DrawRectangleLinesEx(collision.rect, 0.3f, color);
//         } break;
//         case COLLISION_POINT: {
//             // collision.point.x += position->x;
//             // collision.point.y += position->y;
//             int centerX = (int)(collision.point.x + object->width / 2.0);
//             int centerY = (int)(collision.point.y + object->height / 2.0);
//             DrawCircle(centerX, centerY, 5.0f, YELLOW);
//         } break;
//         // case COLLISION_POLYGON:
//         // case COLLISION_POLYLINE: {
//         //     double** points = collision.polygon.points;
//         //     int count       = collision.polygon.count;
//         //     double offset_x = object->x + position->x;
//         //     double offset_y = object->y + position->y;
//         //     if (object->obj_type == OT_POLYGON) {
//         //         DrawTMXPolygon(offset_x, offset_y, points, count, ORANGE);
//         //     }
//         //     if (object->obj_type == OT_POLYLINE) {
//         //         DrawTMXPolyline(offset_x, offset_y, points, count, PURPLE);
//         //     }
//         // } break;
//         // case COLLISION_ELLIPSE: {
//         //     int centerX   = (int)(collision.rect.x + position->x);
//         //     int centerY   = (int)(collision.rect.y + position->y);
//         //     float radiusH = (float)(collision.rect.width);
//         //     float radiusV = (float)(collision.rect.height);
//         //     DrawEllipseLines(centerX, centerY, radiusH, radiusV, GREEN);
//         // } break;
//     }
// }

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
                // CollisionsTMXForeach(m_map, DrawCollisons, NULL);
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

bool Game::checkInteracts(Rectangle bounds, TmxObject collided) {

    for (int i = 0; i < collided.propertiesLength; i++) {
        auto prop = collided.properties[i];

        if (prop.name == "transition") {
            changeMap(prop.stringValue);

            return true;
        }
    }

    return false;
}

float Game::verifyCollisionX(Rectangle bounds, float speedX) {
    TmxObject collided;

    if (CheckCollisionTMXTileLayersRec(m_map, m_map->layers, m_map->layersLength, bounds, &collided)) {
        if (speedX > 0)
            bounds.x = collided.aabb.x - bounds.width;
        else if (speedX < 0)
            bounds.x = collided.aabb.x + collided.aabb.width;
    }

    return bounds.x;
}

float Game::verifyCollisionY(Rectangle bounds, float speedY) {
    TmxObject collided;
    if (CheckCollisionTMXTileLayersRec(m_map, m_map->layers, m_map->layersLength, bounds, &collided)) {
        if (checkInteracts(bounds, collided)) return 0;

        if (speedY > 0)
            bounds.y = collided.aabb.y - bounds.height;
        else if (speedY < 0)
            bounds.y = collided.aabb.y + collided.aabb.height;
    }
    
    return bounds.y;
}

Vector2 Game::findSpawn(TmxMap* map) {
    std::string objname = "spawnpoint";

    TmxLayer objectLayer = TmxLayer();
    for (int i = 0; i < map->layersLength; i++) {
        auto layer = map->layers[i];

        if (strcmp("objects", layer.name) == 0) {
            objectLayer = layer;
            break;
        }
    }

    if (strcmp("objects", objectLayer.name) != 0) return Vector2();

    for (int i = 0; i < objectLayer.exact.objectGroup.objectsLength; i++) {
        auto obj = objectLayer.exact.objectGroup.objects[i];

        if (obj.name == objname) {
            return Vector2(obj.x, obj.y);
        }
    }

    return Vector2();
}