#pragma once

enum Direction {
    UP, DOWN, RIGHT, LEFT
};

enum class Header : char {
    AUTH,
    UPDATE_PLAYER,
    ADD_REMOTE_PLAYER,
    REMOVE_REMOTE_PLAYER,
    UPDATE_REMOTE_PLAYER,
    SEND_MESSAGE
};

struct PlayerState {
    float x, y;
    Direction direction;
    int frame;
};

#pragma pack(push, 1)
struct LoginPacket {
    Header header = Header::AUTH;
    char username[32] = { 0 };
};

struct UpdatePlayerPacket {
    Header header = Header::UPDATE_PLAYER;
    PlayerState state;
};

struct AddRemotePlayerPacket {
    Header header = Header::ADD_REMOTE_PLAYER;
    
    int id;
    char username[32];
    
    PlayerState state;
};

struct RemoveRemotePlayerPacket {
    Header header = Header::REMOVE_REMOTE_PLAYER;
    unsigned int id;
};

struct UpdateRemotePlayerPacket {
    Header header = Header::UPDATE_REMOTE_PLAYER;
    unsigned int id;
    PlayerState state;
};

struct SendMessagePacket {
    Header header = Header::SEND_MESSAGE;
    char message[1024];
};

#pragma pack(pop) 