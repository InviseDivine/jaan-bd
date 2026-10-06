#include "Client.hpp"
#include <Server.hpp>
#include <Types.hpp>
#include <format>

void Client::packetReceived(ENetPacket* packet) {
    auto& srv = Server::get();

    Header header = static_cast<Header>(packet->data[0]);

    switch (header) {
        case Header::AUTH: {
            LoginPacket* pack = reinterpret_cast<LoginPacket*>(packet->data);
            m_username = pack->username;
            srv.clientLogined(this);

            printf("Auth %s\n", pack->username);
            break; 
        }

        case Header::UPDATE_PLAYER: {
            UpdatePlayerPacket* pack = reinterpret_cast<UpdatePlayerPacket*>(packet->data);
            loadPlayerState(pack->state);

            UpdateRemotePlayerPacket updPacket {
                .id = (unsigned int)m_peer->connectID,
                .state = m_state
            };

            srv.broadcast((char*)&updPacket, sizeof(updPacket), this);
            break;
        }

        case Header::SEND_MESSAGE: {
            SendMessagePacket* pack = reinterpret_cast<SendMessagePacket*>(packet->data);
            
            std::string message = std::format("<{}> {}", m_username, pack->message);

            SendMessagePacket broadcastMessage;

            std::copy(message.begin(), message.end(), broadcastMessage.message);
            
            srv.broadcast((char*)&broadcastMessage, sizeof(broadcastMessage), this);

            break;
        }
    }
}

bool Client::sendPacket(char* data, int size) {
    auto enetPacket = enet_packet_create(data, size, ENET_PACKET_FLAG_RELIABLE);

    return enet_peer_send(m_peer, 0, enetPacket) == 0;
}