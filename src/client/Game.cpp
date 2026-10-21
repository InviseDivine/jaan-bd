#define MMNOSOUND
#include "Game.hpp"
#include <raylib.h>
#include <fstream>
#include <Player.hpp>
#include <Timer.hpp>
#include <algorithm>

#include <RemotePlayer.hpp>
#include <Multiplayer.hpp>

#include <utils.h>

#define RRES_IMPLEMENTATION
#include "rres.h"
#define RRES_RAYLIB_IMPLEMENTATION
#include "rres-raylib.h"

rresCentralDir dir;

#define RRES_FILENAME "resources.rres"

const char* LoadTextFilePacked(const char* path) {
    rresResourceChunk chunk = { 0 };

    chunk = rresLoadResourceChunk(RRES_FILENAME, rresGetResourceId(dir, GetFileName(path)));
    int result = UnpackResourceChunk(&chunk);

    if (result == 0) {
        const char* data = (const char*)LoadTextFromResource(chunk);
        return data;
    }

    rresUnloadResourceChunk(chunk);

    return nullptr;
}


Texture2D LoadTexturePacked(const char* path) {
    rresResourceChunk chunk = { 0 };

    chunk = rresLoadResourceChunk(RRES_FILENAME, rresGetResourceId(dir, GetFileName(path)));
    int result = UnpackResourceChunk(&chunk);
    Texture2D ret = {};

    if (result == 0) {
        unsigned int dataSize = 0;
        const unsigned char* data = (const unsigned char*)LoadDataFromResource(chunk, &dataSize);
        Image image = LoadImageFromMemory(GetFileExtension(path), data, dataSize);

        if (image.data != NULL) {
            ret = LoadTextureFromImage(image);
            UnloadImage(image);
        }
    }

    rresUnloadResourceChunk(chunk);

    return ret;
}

Font LoadFontPacked(const char* path, int fontSize, int* codepoint, int codepointLen) {
    rresResourceChunk chunk = { 0 };

    chunk = rresLoadResourceChunk(RRES_FILENAME, rresGetResourceId(dir, GetFileName(path)));
    int result = UnpackResourceChunk(&chunk);
    Font ret = { 0 };

    if (result == 0) {
        unsigned int dataSize = 0;
        const unsigned char* data = (const unsigned char*)LoadDataFromResource(chunk, &dataSize);
        ret = LoadFontFromMemory(GetFileExtension(path), data, dataSize, fontSize, codepoint, codepointLen); 
    }

    return ret;
}

Sound LoadSoundPacked(const char* path) {
    rresResourceChunk chunk = { 0 };

    chunk = rresLoadResourceChunk(RRES_FILENAME, rresGetResourceId(dir, GetFileName(path)));
    int result = UnpackResourceChunk(&chunk);
    Sound ret = { 0 };

    if (result == 0) {
        unsigned int dataSize = 0;
        const unsigned char* data = (const unsigned char*)LoadDataFromResource(chunk, &dataSize);
        Wave wave = LoadWaveFromMemory(GetFileExtension(path), data, dataSize);
        ret = LoadSoundFromWave(wave);
        UnloadWave(wave); 
    }

    rresUnloadResourceChunk(chunk);

    return ret;
}

Music LoadMusicStreamPacked(const char* path) {
    rresResourceChunk chunk = { 0 };

    chunk = rresLoadResourceChunk(RRES_FILENAME, rresGetResourceId(dir, GetFileName(path)));
    int result = UnpackResourceChunk(&chunk);
    Music ret = { 0 };

    if (result == 0) {
        unsigned int dataSize = 0;
        const unsigned char* data = (const unsigned char*)LoadDataFromResource(chunk, &dataSize);
        ret = LoadMusicStreamFromMemory(GetFileExtension(path), data, dataSize);
    }

    rresUnloadResourceChunk(chunk);

    return ret;
}

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

    auto prop = findProperty(map, "background_music");
    auto zoom = findProperty(map, "zoom");

    if (zoom != nullptr && zoom->floatValue != 0 && m_username == "jaan") {
        cam.zoom = zoom->floatValue;
    }

    if (name != "end") {
        PlaySound(m_sounds["snd_dooropen.wav"]);
        PlaySound(m_sounds["snd_doorclose.wav"]);
    } else {
        m_showPlayers = true;
    }

    if (prop != nullptr) {
        m_currentTrack = &m_music[prop->stringValue];
        m_currentTrack->looping = true;
        SetMusicVolume(*m_currentTrack, 0.3f);
        SeekMusicStream(*m_currentTrack, 0.f);
        PlayMusicStream(*m_currentTrack);
    } else {
        m_currentTrack = nullptr;
    }

    m_map = map;
    m_player->setPoisition(findSpawn(map, spawnpoint));
    m_player->resetSpeed();

    auto& mult = Multiplayer::get();
    mult.mapChanged(name);
}

void Game::run() {
    InitWindow(1280, 720, "JaanBD");
    SetTargetFPS(60);
    InitAudioDevice();

    dir = rresLoadCentralDirectory("resources.rres");

    SetGetFileTextCallbackTMX(LoadTextFilePacked);
    SetLoadTextureCallbackTMX(LoadTexturePacked);

    std::string codepointsText = "АаБбВвГгДдЕеЁёЖжЗзИиЙйКкЛлМмНнОоПпРрСсТтУуФфХхЦцЧчШшЩщЪъЫыЬьЭэЮюЯяAaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz1234567890?!.,()[]{}?/\\|-+=<>:;";
    int len = 0;
    int* codepoints = LoadCodepoints(codepointsText.c_str(), &len);

    m_marksman = LoadFontPacked("Marksman.otf", 30, codepoints, len);
    SetTextureFilter(m_marksman.texture, TEXTURE_FILTER_POINT);
    UnloadCodepoints(codepoints);

    m_candle = LoadTexturePacked("candle.png");
    
    SetExitKey(KEY_NULL);

    loadMap("house");
    loadMap("kolyahhouse");
    loadMap("invdivhouse");
    loadMap("fullharmonyhouse");
    loadMap("delhouse");
    loadMap("danilkahouse");

    loadMap("end");
    loadMap("untitled");
    loadMap("basement");

    m_map = findMap("house");
    
    cam = {
        .offset = Vector2{1280/2, 720/2},
        .target = Vector2{0.f, 0.f},
        .rotation = 0.f,
        .zoom = 5.f
    };

    TraceLog(LOG_INFO, "Map size in pixels %ix%i", m_map->tileWidth * m_map->width, m_map->tileHeight * m_map->height);
    SetMasterVolume(0.5f);

    std::string username;
    if (FileExists("username.txt")) {
        username = TextToLower(LoadFileText("username.txt"));
        m_reconnectingVisible = true;
        m_showPlayers = true;
    } else {
        username = "jaan";
        m_reconnectingVisible = false;
        m_showPlayers = false;
    }

    m_username = username;
    std::string texName = /* "assets/" + */ username + ".png";

    Texture2D plTex = LoadTexturePacked(texName.c_str());

    m_notesAvatar["note"] = LoadTexturePacked("paper.png");
    m_notesAvatar["noteinsc"] = LoadTexturePacked("paperinsc.png");
    m_notesAvatar["danilka"] = LoadTexturePacked("danilka.png");

    #define LoadSnd(name) m_sounds[name] = LoadSoundPacked(##name)

    LoadSnd("meow.mp3");
    LoadSnd("snd_doorclose.wav");
    LoadSnd("snd_dooropen.wav");

    #define LoadMsc(name) m_music[name] = LoadMusicStreamPacked(##name)

    LoadMsc("temple_of_magics.mp3");
    LoadMsc("avril14.mp3");
    LoadMsc("californication.mp3");
    LoadMsc("wackyloop.mp3");
    LoadMsc("alberto_balsalm.mp3");
    LoadMsc("stardewvibes.mp3");

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

    while (!WindowShouldClose()) {
        if (m_currentTrack != nullptr) {
            UpdateMusicStream(*m_currentTrack);
        }

        if (m_showChat) {
            if (IsKeyPressed(KEY_ESCAPE)) m_showChat = false;
            int key = GetCharPressed();

            // Check if more characters have been pressed on the same frame
            while (key > 0) {
                int size = 0;
                m_message.append(CodepointToUTF8(key, &size));

                key = GetCharPressed(); // Check next character in the queue
            }

            if (IsKeyPressedRepeat(KEY_BACKSPACE) || IsKeyPressed(KEY_BACKSPACE)) {
                if (m_message.size() != 0) {
                    int size = 0;
                    auto codepoint = GetCodepointPrevious(m_message.data() + m_message.size(), &size);

                    m_message = m_message.substr(0, m_message.size() - size);
                }
            }

            if (IsKeyPressed(KEY_ENTER)) {
                auto& mp = Multiplayer::get();

                mp.sendMessage(m_message);
                m_message.clear();
                m_showChat = false;
            }
        }
        timer.advanceTime();

        if (IsKeyPressed(KEY_T) && m_showPlayers && !m_showChat) m_showChat ^= 1;

        if (!m_showChat && IsKeyPressed(KEY_R) && m_username != "jaan") {
            changeMap("end");
        }

        if (!m_showDialog && !m_showChat) {
            for (int i = 0; i < timer.getTicks(); i++) {
                m_player->onTick();
            }
        }
        
        if (m_damageCooldown > 0) {
            m_damageCooldown -= 5;
        } else {
            m_damageCooldown = 0;
        }

        if (m_hp <= 0) {
            m_hp = 20;
            m_player->setPoisition(findSpawn(m_map));
        }

        Vector2 position = m_player->getPosition();

        cam.target = position;

        // ----- VIBECODE AHTUNG!!! -----
        // kolyah35: bro this is not vibe code just ai assist
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

            if (m_showPlayers) {
                for (auto& [id, remote] : m_remotes) {
                    players.push_back(remote);
                }
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

                auto layer = findLayer(m_map, "cake");

                if (layer && layer->visible) {
                    for (int i = 0; i < m_collectedCandles; i++) {
                        // SYKA CHTO DELAET ETOT TIP?!
                        switch(i) {
                            case 0: {
                                DrawTextureRec(m_candle, {7, 3, 1, 9}, {474, 299 - 9}, WHITE);
                                break;
                            }

                            case 1: {
                                DrawTextureRec(m_candle, {7, 3, 1, 9}, {483, 299 - 9}, WHITE);
                                break;
                            }

                            case 2: {
                                DrawTextureRec(m_candle, {7, 3, 1, 9}, {476, 301 - 9}, WHITE);
                                break;
                            }

                            case 3: {
                                DrawTextureRec(m_candle, {7, 3, 1, 9}, {481, 301 - 9}, WHITE);
                                break;
                            }

                            case 4: {
                                DrawTextureRec(m_candle, {7, 3, 1, 9}, {478, 300 - 9}, WHITE);
                                break;
                            }
                        }
                    }
                }
                    
                for (auto pl : players) pl->draw();
                DrawRectangleLinesEx(m_player->getBounds(), 0.3f, RED);
            EndMode2D();

            if (m_reconnectingVisible) { // Connecting state
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

            if (m_showDialog && m_currentMessage == m_dialogMessage.size()) {
                m_showDialog = false;
            }

            if (m_showDialog) {
                float scale = !m_currentNote.empty() ? 2.f : 0;

                static float index = 0;
                if (m_currentPos < m_dialogMessage[m_currentMessage].length()) {
                    // int size = 0;
                    // int cp = GetCodepointNext(m_dialogMessage[m_currentMessage].data() + m_currentPos, &size);
                    m_currentPos++;

                    if (!m_currentNote.empty() && m_currentPos % 4 == 0) {
                        index++;

                        if (index > m_notesAvatar.at(m_currentNote).width / 64) {
                            index = 0;
                        }
                    }
                }

                float padding = 10.f;
                float width = GetScreenWidth() - padding * 2;
                float height = 200.f;
                int thick = 5;
                
                float x = padding;
                float y = GetScreenHeight() - height - padding;
                DrawRectangle(x + thick, y + thick, width - thick, height - thick, BLACK);
                DrawRectangleLinesEx({x, y, width, height}, thick, WHITE);

                Vector2 pos = {x + thick + padding + 64 * scale, y + thick + padding * 2};

                DrawTextBoxed(
                    m_marksman, 
                    TextSubtext(m_dialogMessage[m_currentMessage].c_str(), 0, m_currentPos), 
                    {pos.x, pos.y, width - thick - padding * 2 - 64 * scale, height - thick - padding * 2}, 
                    30, 1.f, true, WHITE
                ); 
                
                if (!m_currentNote.empty()) {
                    auto& tex = m_notesAvatar.at(m_currentNote);
                    if (m_currentPos < m_dialogMessage[m_currentMessage].length()) {
                        DrawTexturePro(tex, 
                            {index * 64, index * 64, 64, 64}, 
                            {x + thick + padding + (64 * scale) / 2, y + thick + padding + 16 + (64 * scale) / 2, 64 * scale, 64 * scale}, 
                            {(64 * scale) / 2, (64 * scale) / 2}, 
                            7.f * sin(GetTime() * 1.7f), 
                            WHITE
                        );
                    } else {
                        DrawTexturePro(tex, 
                            {0, 0, 64, 64},
                            {x + thick + padding + (64 * scale) / 2, y + thick + padding + 16 + (64 * scale) / 2, 64 * scale, 64 * scale}, 
                            {(64 * scale) / 2, (64 * scale) / 2}, 
                            7.f * sin(GetTime() * 1.7f), 
                            WHITE
                        );
                    }
                }

                if ((IsKeyPressed(KEY_E) || IsKeyPressed(KEY_SPACE)) && m_currentPos > 1) {
                    if (m_currentPos < m_dialogMessage[m_currentMessage].length()) m_currentPos = m_dialogMessage[m_currentMessage].length();
                    else if (m_currentPos == m_dialogMessage[m_currentMessage].length()) {
                        m_currentMessage++;
                        m_currentPos = 0;
                    }
                }
            }

            if (m_counterVisible && !m_showDialog) {
                // решил сделать фокус членом
                rlPushMatrix();
                rlScalef(3.f, 3.f, 0.f);
                DrawTextureRec(m_candle, {0, 0, 16, 12}, {10, 10}, WHITE);
                DrawText(TextFormat("%d/6", m_collectedCandles), 25, 12, 10, WHITE);
                rlPopMatrix();
            }

            if (m_showPlayers) {
                for (float i = 0; i < m_messages.size(); i++) {
                    DrawTextEx(m_marksman, m_messages.at(i).c_str(), {0, i * 27}, 25.f, 1.f, WHITE);
                }
            }
            
            if (m_showChat && m_showPlayers) {
                float height = 50.f;
                float width = GetScreenWidth();
                float y = GetScreenHeight() - height;
                float padding = 5.f;

                DrawRectangle(0, y, width, height, {0, 0, 0, 130});
                DrawTextEx(m_marksman, m_message.c_str(), {padding, (height - 25) / 2 + y}, 25.f, 1.f, WHITE);
            }

            EndDrawing();
    }
}

bool Game::addRemotePlayer(AddRemotePlayerPacket& packet) {
    std::string username = packet.username;
    std::string filename = /* "assets/" + */ std::string(TextToLower(packet.username)) + ".png";

    TraceLog(LOG_INFO, "Loading skin %s", filename.c_str());

    Texture2D skin = LoadTexturePacked(filename.c_str());

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
                if (strcmp(prop.stringValue, "end") == 0) {
                    if (m_username == "jaan" && m_collectedCandles < 6) return false;
                }

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
                m_currentNote.clear();
                for (int x = 0; x < collided.propertiesLength; x++) {
                    auto avatar = collided.properties[x];

                    if (strcmp(avatar.name, "avatar") == 0) {
                        m_currentNote = avatar.stringValue;
                        break;
                    }
                }

                m_currentPos = 0;
                m_currentMessage = 0;
                m_dialogMessage.clear();

                int lines = 0;
                char** dialogues = TextSplit(prop.stringValue, '\n', &lines);
                for (int i = 0; i < lines; i++) {
                    m_dialogMessage.push_back(dialogues[i]);
                }

                m_showDialog = true;

                return true;
            }

            if (strcmp(prop.name, "candle") == 0 && m_username == "jaan") {
                m_collectedCandles++;
                
                for (int i = 0; i < objectLayer->exact.objectGroup.objectsLength; i++) {
                    if (objectLayer->exact.objectGroup.objects[i].id == collided.id) {
                        for (int j = i; j < objectLayer->exact.objectGroup.objectsLength-1; j++)
                            objectLayer->exact.objectGroup.objects[j] = objectLayer->exact.objectGroup.objects[j+1];
                        
                        objectLayer->exact.objectGroup.objectsLength--;
                    }
                }

                findLayer(m_map, "candles")->visible = false;
                return true;
            } 

            if (strcmp(prop.name, "activate_counter") == 0 && (IsKeyPressed(KEY_E) || IsKeyPressed(KEY_ENTER))) {
                m_counterVisible = prop.boolValue;
                continue;
            }

            if (strcmp(prop.name, "play_sound") == 0 && (IsKeyPressed(KEY_E) || IsKeyPressed(KEY_ENTER))) {
                PlaySound(m_sounds[prop.stringValue]);
                continue;
            }

            if (strcmp(prop.name, "damage") == 0 && m_username == "jaan" && m_damageCooldown == 0) {
                m_damageCooldown = 3 * 60;
                m_hp -= prop.intValue;
                printf("damage! \n");
                break;
            }

            if (strcmp(prop.name, "cake") == 0 && m_username == "jaan" && m_collectedCandles == 6) {
                if (prop.boolValue) {
                    m_player->tookCake();
                    auto layer = findLayer(m_map, "cake");

                    if (layer) layer->visible = false;
                } else {
                    m_player->placeCake();
                    auto layer = findLayer(m_map, "cake");

                    if (layer) layer->visible = true;
                }
                
                break;
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

TmxProperty* Game::findProperty(TmxMap* map, std::string const& name) {
    for (int i = 0; i < map->propertiesLength; i++) {
        if (map->properties[i].name == name) {
            return &map->properties[i];
        }
    }

    return nullptr;
}