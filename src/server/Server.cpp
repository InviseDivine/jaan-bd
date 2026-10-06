#include <Server.hpp>
#include <iostream>
#include <Client.hpp>
#include <unordered_map>

void Server::run() {
    if (enet_initialize() != 0) {
        printf("An error occurred while initializing ENet.\n");
        return;
    }

    ENetAddress address = {0};

    address.host = ENET_HOST_ANY; /* Bind the server to the default localhost.     */
    address.port = 7777; /* Bind the server to port 7777. */

    /* create a server */
    ENetHost* server = enet_host_create(&address, 255, 2, 0, 0);

    if (server == NULL) {
        printf("An error occurred while trying to create an ENet server host.\n");
        return;
    }

    printf("Started a server...\n");

    ENetEvent event;


    while (true) {
        /* Wait up to 1000 milliseconds for an event. (WARNING: blocking) */
        while (enet_host_service(server, &event, 1000) > 0) {
            auto id = event.peer->connectID;

            switch (event.type) {
                case ENET_EVENT_TYPE_CONNECT: {
                    printf("A new client connected from %x:%u.\n",  event.peer->address.host, event.peer->address.port);
                    if (!m_clients.contains(id)) {
                        event.peer->data = (void*)id;
                        Client* cl = new Client(event.peer);

                        m_clients[id] = cl;
                    }

                    break;
                }

                case ENET_EVENT_TYPE_RECEIVE:
                    // printf("A packet of length %lu.\n", event.packet->dataLength);
                    if (m_clients.contains(id)) {
                        m_clients[id]->packetReceived(event.packet);
                    }
                        
                    enet_packet_destroy(event.packet);
                    break;

                case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
                case ENET_EVENT_TYPE_DISCONNECT: {
                    printf("disconnected.\n");
                    /* Reset the peer's client information. */
                    event.peer->data = NULL;
                    auto username = m_clients[event.peer->connectID]->getUsername();

                    if (!username.empty()) {
                        m_savedPlayers[username] = m_clients[event.peer->connectID]->getPlayerState(); 
                    }

                    RemoveRemotePlayerPacket packet{
                        .id = event.peer->connectID
                    };
                    
                    broadcast((char*)&packet, sizeof(packet));

                    delete m_clients[event.peer->connectID];
                    m_clients.erase(event.peer->connectID);
                    break;
                }

                case ENET_EVENT_TYPE_NONE:
                    break;
            }
        }
    }

    enet_host_destroy(server);
    enet_deinitialize();
}

void Server::broadcast(char* data, int len, Client* exclude) {
    for (auto& cl : m_clients) if (cl.second != exclude) cl.second->sendPacket(data, len);
}

void Server::clientLogined(Client* cl) {
    auto username = cl->getUsername();

    if (m_savedPlayers.contains(username)) {
        cl->loadPlayerState(m_savedPlayers[username]);
    }

    {
        AddRemotePlayerPacket packet{
            .id = cl->getId(),
            .state = cl->getPlayerState()
        };

        std::copy(username.begin(), username.end(), packet.username);

        broadcast((char*)&packet, sizeof(packet), cl);
    }

    for (auto& [us, client] : m_clients) {
        if (client == cl) continue;

        auto us = client->getUsername();

        AddRemotePlayerPacket packet{
            .id = client->getId(),
            .state = client->getPlayerState()
        };

        std::copy(us.begin(), us.end(), packet.username);

        cl->sendPacket((char*)&packet, sizeof(packet));
    }
}