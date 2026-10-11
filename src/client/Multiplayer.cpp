#include <Multiplayer.hpp>
#include <cstdio>
#include <iostream>
#define NOGDI
#define NOUSER
#define PlaySoundA PlaySoundAWin32
#include <enet.h>
#include <iostream>
#include <Types.hpp>
#include <Game.hpp>
#include <raylib.h>

#define SEND_PACKET(packet) sendPacket((char*)&packet, sizeof(packet))

void Multiplayer::disconnect() {
    enet_peer_disconnect(m_peer, 0);
}

void Multiplayer::init(std::string nickname, std::string ip, int port) {
    if (nickname.length() > 32) {
        fprintf(stderr, "Username is too long.\n");
        exit(EXIT_FAILURE);
    }

    m_username = nickname;

    std::cout << m_username << std::endl;

    if (enet_initialize() != 0) {
        fprintf(stderr, "An error occurred while initializing ENet.\n");
        exit(EXIT_FAILURE);
    }
    
    m_client = enet_host_create (NULL /* create a client host */,
            1 /* only allow 1 outgoing connection */,
            2 /* allow up 2 channels to be used, 0 and 1 */,
            0 /* assume any amount of incoming bandwidth */,
            0 /* assume any amount of outgoing bandwidth */);

    if (m_client == NULL) {
        exit(EXIT_FAILURE);
    }

    if (enet_address_set_host(&m_address, ip.c_str()) != 0) {
        fprintf(stderr, "Failed to resolve host %s\n", ip.c_str());

        return;
    }    

    m_address.port = port;
}

void Multiplayer::update() {
    if (!m_connected && GetTime() > m_timeout + 10.f) {
        m_peer = enet_host_connect(m_client, &m_address, 2, 0);
        m_timeout = GetTime();
        m_reconnects++;

        if (m_peer == NULL) {
            printf("peer is null. Reconnecting...\n");
            return;
        }
    }

    ENetEvent event;

    while(enet_host_service(m_client, &event, 0) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_RECEIVE: {
                handlePacket(event.packet);
                enet_packet_destroy(event.packet);

                break;
            }

            case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
            case ENET_EVENT_TYPE_DISCONNECT: {
                printf("Disconnected. Reconnecting...\n");
                m_connected = false;

                enet_peer_reset(m_peer);

                Game::get().removeAllRemotePlayers();

                break;
            }
 
            case ENET_EVENT_TYPE_NONE:
                break;
            case ENET_EVENT_TYPE_CONNECT:
                printf("Connected!\n");
                m_connected = true;

                auto& game = Game::get();
                auto map = game.currentMap();

                LoginPacket loginPacket;
                std::copy(m_username.begin(), m_username.end(), loginPacket.username);
                printf("My username: %s %s", m_username.c_str(), loginPacket.username);

                std::copy(map->fileName, map->fileName + TextFindIndex(map->fileName, ".tmx")-1, loginPacket.map);

                SEND_PACKET(loginPacket);
                break;
        }
    }
}

void Multiplayer::handlePacket(ENetPacket* packet) {     
    Header header = static_cast<Header>(packet->data[0]);
    auto& game = Game::get();

    switch (header) {
        case Header::ADD_REMOTE_PLAYER: {
            AddRemotePlayerPacket* pack = (AddRemotePlayerPacket*)packet->data;
            game.addRemotePlayer(*pack);

            printf("Added new player %lu and username %s\n", pack->id, pack->username);
            
            break;
        }

        case Header::UPDATE_REMOTE_PLAYER: {
            UpdateRemotePlayerPacket* pack = (UpdateRemotePlayerPacket*)packet->data;
            game.updateRemotePlayer(pack->id, pack->state);
            
            break;
        }

        case Header::REMOVE_REMOTE_PLAYER: {
            RemoveRemotePlayerPacket* pack = (RemoveRemotePlayerPacket*)packet->data;
            game.removeRemotePlayer(pack->id);

            printf("Removed player %lu\n", pack->id);
            
            break;
        }
        
        case Header::SEND_MESSAGE: {
            SendMessagePacket* pack = (SendMessagePacket*)packet->data;
            game.addMessage(pack->message);
            
            break;
        };
    }
}

void Multiplayer::sendPacket(char* data, int size) {
    if (m_connected) {
        ENetPacket* packet = enet_packet_create(data, size, ENET_PACKET_FLAG_RELIABLE);

        enet_peer_send(m_peer, 0, packet);
    }
}

void Multiplayer::mapChanged(std::string const& to) {
    ChangeMapPacket packet;
    memcpy(packet.name, to.data(), to.size());

    sendPacket((char*)&packet, sizeof(packet));
}