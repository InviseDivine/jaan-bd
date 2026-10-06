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

void DrawCollisons(tmx_object *object, RaylibTMXCollision collision, void* userdata) {
    // Vector2* position = (Vector2*)userdata;
    switch (collision.type)
    {
        case COLLISION_RECT: {
            Color color = object->obj_type == OT_TILE ? RED : BLUE;
            // collision.rect.x += position->x;
            // collision.rect.y += position->y;
            DrawRectangleLinesEx(collision.rect, 0.3f, color);
        } break;
        case COLLISION_POINT: {
            // collision.point.x += position->x;
            // collision.point.y += position->y;
            int centerX = (int)(collision.point.x + object->width / 2.0);
            int centerY = (int)(collision.point.y + object->height / 2.0);
            DrawCircle(centerX, centerY, 5.0f, YELLOW);
        } break;
        // case COLLISION_POLYGON:
        // case COLLISION_POLYLINE: {
        //     double** points = collision.polygon.points;
        //     int count       = collision.polygon.count;
        //     double offset_x = object->x + position->x;
        //     double offset_y = object->y + position->y;
        //     if (object->obj_type == OT_POLYGON) {
        //         DrawTMXPolygon(offset_x, offset_y, points, count, ORANGE);
        //     }
        //     if (object->obj_type == OT_POLYLINE) {
        //         DrawTMXPolyline(offset_x, offset_y, points, count, PURPLE);
        //     }
        // } break;
        // case COLLISION_ELLIPSE: {
        //     int centerX   = (int)(collision.rect.x + position->x);
        //     int centerY   = (int)(collision.rect.y + position->y);
        //     float radiusH = (float)(collision.rect.width);
        //     float radiusV = (float)(collision.rect.height);
        //     DrawEllipseLines(centerX, centerY, radiusH, radiusV, GREEN);
        // } break;
    }
}

bool Game::loadMap(std::string const& name) {
    auto filename = "assets/" + name + ".tmx";
    auto map = LoadTMX(filename.c_str());

    if (!map) return false;

    m_maps[name] = map;
    return true;
}

tmx_map* Game::findMap(std::string const& name) {
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

    TraceLog(LOG_INFO, "Map size in pixels %ix%i", m_map->tile_width * m_map->width, m_map->tile_height * m_map->height);

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

    tmx_layer* layer = tmx_find_layer_by_name(m_map, "Collidable");
    if (layer) layer->visible = false;

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
            m_map->tile_width * (m_map->width - 1) - (cam.offset.x / cam.zoom),
            m_map->tile_height * (m_map->height - 1) - (cam.offset.y / cam.zoom)
        };

        if (min.x < max.x && min.y < max.y) {
            cam.target = {
                std::clamp(position.x, min.x, max.x),
                std::clamp(position.y, min.x, max.y)
            };
        } else {
            cam.target = {
                (float)m_map->tile_width * (m_map->width - 1) / 2,
                (float)m_map->tile_height * (m_map->height - 1) / 2
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
                DrawTMX(m_map, m_map->tile_width, m_map->tile_width, WHITE, 1.0f);
                
                for (auto pl : players) pl->draw();

                CollisionsTMXForeach(m_map, DrawCollisons, NULL);
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

struct Collider {
    Rectangle bounds;
    float speed;
};

void UpdateCollisionX(tmx_object *object, RaylibTMXCollision collision, void* userdata) {
    Collider* collider = (Collider*)userdata;
    
    if (collision.type == COLLISION_RECT) {
        if (CheckCollisionRecs(collider->bounds, collision.rect)) {
            if (collider->speed > 0)
                collider->bounds.x = collision.rect.x - collider->bounds.width;
            else if (collider->speed < 0)
                collider->bounds.x = collision.rect.x + collision.rect.width;
        }
    }
}

void UpdateCollisionY(tmx_object *object, RaylibTMXCollision collision, void* userdata) {
    Collider* collider = (Collider*)userdata;

    if (collision.type == COLLISION_RECT) {
        if (CheckCollisionRecs(collider->bounds, collision.rect)) {
            if (collider->speed > 0)
                collider->bounds.y = collision.rect.y - collider->bounds.height;
            else if (collider->speed < 0)
                collider->bounds.y = collision.rect.y + collision.rect.height;
        }
    }
}

float Game::verifyCollisionX(Rectangle bounds, float speedX) {
    Collider coll = {bounds, speedX};

    CollisionsTMXForeach(m_map, UpdateCollisionX, &coll);

    return coll.bounds.x;
}

float Game::verifyCollisionY(Rectangle bounds, float speedY) {
    Collider coll = {bounds, speedY};

    CollisionsTMXForeach(m_map, UpdateCollisionY, &coll);

    return coll.bounds.y;
}

Vector2 Game::findSpawn(tmx_map* map) {
    std::string objname = "spawnpoint";

    auto objects = tmx_find_layer_by_name(map, "objects");

    if (!objects) return Vector2();

    auto obj = objects->content.objgr->head;
    while (obj) {
        if (obj->name && std::string(obj->name) == objname) {
            return Vector2(obj->x, obj->y);
        }

        obj = obj->next;
    }

    return Vector2();
}

void UpdateCollisionInteracts(tmx_object *object, RaylibTMXCollision collision, void* userdata) {
    if (collision.type == COLLISION_RECT) {
        if (CheckCollisionRecs(*(Rectangle*)userdata, collision.rect)) {
            auto trans = tmx_get_property(object->properties, "transition");

            if (trans != nullptr) {
                TraceLog(LOG_INFO, "Transition!");
                return Game::get().changeMap(trans->value.string);
            }
        }
    }
}

void Game::checkInteracts(Rectangle bounds) {
    CollisionsTMXForeach(m_map, UpdateCollisionInteracts, &bounds);
}