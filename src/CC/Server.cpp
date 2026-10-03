#include "Server.hpp"
#include "CSMessage.hpp"
#include "Utils.hpp"
namespace cc
{
    void Server::Start(unsigned short port)
    {
        if (listener.listen(port) != sf::Socket::Status::Done)
        {
            std::cerr << "Failed to listen on port " << port << '\n';
            return;
        }

        listener.setBlocking(false);

        std::cout << "Server listening on port " << port << '\n';
    }
    Server::Server()
    {
        planets.push_back(std::make_unique<Planet>());
        planets[0]->isServerPlanet = true;
    }
    void Server::SetSeed(uint64_t seed)
    {
        this->seed = seed;
        for (int i = 0; i < planets.size(); i++)
        {
            planets[i]->SetSeed(seed + i);
        }
    }
    void Server::Tick()
    {
        for (auto &p : planets)
        {
            p->Tick();
        }
    }
    uint64_t Server::GetSeed()
    {
        return seed;
    }
    void Server::Update(double dt)
    {
        AcceptClients();
        ReceivePackets();
        timeSinceTick += dt;
        if (timeSinceTick > (1.0 / tps))
        {
            Tick();
            timeSinceTick -= dt;
        }
    }
    uint64_t Server::GetNextClientId()
    {
        return currClientId++;
    }

    void Server::SendToClient(uint64_t clientId, sf::Packet &packet)
    {
        // TODO: add this
        //  if (clientID == ((Client *)state.get())->player.id)
        //  {
        //      ((Client *)state.get())->ProcessPacket(packet);
        //      return;
        //  }
        for (auto &c : clients)
        {
            if (c.id == clientId)
            {
                c.socket.send(packet);
                break;
            }
        }
    }
    void Server::Broadcast(sf::Packet &packet, std::vector<uint64_t> exclusions)
    {
        for (auto &client : clients)
        {
            if (std::find(exclusions.begin(), exclusions.end(), client.id) == exclusions.end())
            {
                // TODO: add this
                //  if (client.id == ((Client*)state.get())->player.id)
                //  {
                //      ((Client*)state.get())->ProcessPacket(packet);
                //      continue;
                //  }
                client.socket.send(packet);
            }
        }
    }

    void Server::AcceptClients()
    {
        while (true)
        {
            ServerClient client;

            sf::Socket::Status status = listener.accept(client.socket);

            if (status == sf::Socket::Status::NotReady)
                break;

            if (status != sf::Socket::Status::Done)
                continue;

            // client.id = clients.size();

            client.socket.setBlocking(false);
            client.id = GetNextClientId();
            clients.push_back(std::move(client));
            BroadcastServerLog("Client connected: " + std::to_string(clients.back().id));
        }
    }
    void Server::ReceivePackets()
    {
        for (std::size_t i = 0; i < clients.size();)
        {
            sf::Packet packet;

            sf::Socket::Status status = clients[i].socket.receive(packet);

            if (status == sf::Socket::Status::Done)
            {
                HandlePacket(clients[i].id, packet);

                ++i;
            }
            else if (status == sf::Socket::Status::NotReady)
            {
                ++i;
            }
            else
            {
                // Disconnected / error
                BroadcastServerLog("Client disconnected: " + std::to_string(clients[i].id));

                clients.erase(clients.begin() + i);
                std::string username = currPlayers[i].username;
                sf::Packet p;
                p << (uint16_t)CSMessageType::PLAYER_LEFT;
                p << username;
                Broadcast(p);
                RegisterCurrentPlayers();
                currPlayers.erase(currPlayers.begin() + i);
            }
        }
    }
    void Server::HandlePacket(std::uint64_t clientId, sf::Packet &packet)
    {
        uint16_t t;
        packet >> t;
        CSMessageType type = (CSMessageType)t;
        if (type == CSMessageType::SEND_USERNAME)
        {
            SendJoinData(clientId, packet);
        }
        else if (type == CSMessageType::CHAT_MESSAGE)
        {
            std::string message;
            packet >> message;
            BroadcastChatLog(message, clientId);
        }
        else if (type == CSMessageType::REQUEST_CHUNKS)
        {
            uint64_t n;
            packet >> n;
            std::vector<sf::Vector2i> positions;
            positions.reserve(n);
            for (int i = 0; i < n; i++)
            {
                sf::Vector2i pos;
                packet >> pos.x >> pos.y;
                positions.push_back(pos);
            }
            SendChunks(clientId, positions);
        }
        else if (type == CSMessageType::REQUEST_SET_TILES)
        {
            // create clone of packet to send to other players
            sf::Packet out;
            out << (uint16_t)CSMessageType::SET_TILES;

            const uint8_t *data = static_cast<const uint8_t *>(packet.getData());
            std::size_t pos = packet.getReadPosition(); // just past the type you already read
            out.append(data + pos, packet.getDataSize() - pos);
            int n;
            packet >> n;
            int planetIndex = currPlayers[GetIndexOfId(clientId)].planet;
            for (int i = 0; i < n; i ++)
            {
                sf::Vector2i position;
                packet >> position.x >> position.y;
                sf::Vector2i chunkPos = TileToChunkPos(position);
                if (!planets[planetIndex]->chunks.contains(chunkPos))
                {
                    planets[planetIndex]->GenerateChunk(chunkPos);
                }
                uint16_t tileType;
                packet >> tileType;
                bool hasTileEntity;
                packet >> hasTileEntity;
                TileEntity *e = nullptr;
                if (hasTileEntity)
                {
                    auto data = ReadBytesFromPacket(packet);
                    Serializer s(Serializer::Mode::READ, Serializer::Format::BINARY, {}, data);
                    e = CreateTileEntityFromType(tileType);
                    e->Serialize(s);
                }
                planets[currPlayers[GetIndexOfId(clientId)].planet]->SetTileAt(position, Tile(tileType), e);
            }

            for (int i = 0; i < currPlayers.size(); i++)
            {
                if (currPlayers[i].planet == planetIndex && clients[i].id != clientId)
                {
                    SendToClient(clients[i].id, out);
                }
            }
        }
        else if (type == CSMessageType::REQUEST_ADD_ENTITIES)
        {
            sf::Packet out;
            out << (uint16_t)CSMessageType::ADD_ENTITIES;
            const uint8_t *data = static_cast<const uint8_t *>(packet.getData());
            std::size_t pos = packet.getReadPosition(); // just past the type you already read
            out.append(data + pos, packet.getDataSize() - pos);
            int planetIndex = currPlayers[GetIndexOfId(clientId)].planet;
            for (int i = 0; i < currPlayers.size(); i++)
            {
                if (currPlayers[i].planet == planetIndex && clients[i].id != clientId)
                {
                    SendToClient(clients[i].id, out);
                }
            }
            int n;
            packet >>n;
            for (int i = 0; i < n; i ++)
            {
                Entity* e = LoadEntityFromPacket(packet);
                planets[currPlayers[GetIndexOfId(clientId)].planet]->AddEntity(e, true);
            }
        }else if (type == CSMessageType::REQUEST_UPDATE_ENTITIES)
        {
            sf::Packet out;
            out << (uint16_t)CSMessageType::UPDATE_ENTITIES;
            const uint8_t *data = static_cast<const uint8_t *>(packet.getData());
            std::size_t pos = packet.getReadPosition(); // just past the type you already read
            out.append(data + pos, packet.getDataSize() - pos);
            int planetIndex = currPlayers[GetIndexOfId(clientId)].planet;
            for (int i = 0; i < currPlayers.size(); i++)
            {
                if (currPlayers[i].planet == planetIndex && clients[i].id != clientId)
                {
                    SendToClient(clients[i].id, out);
                }
            }
            int n;
            packet >> n;
            for (int i = 0; i < n; i ++)
            {
                int index;
                packet >> index;
                Entity* e = LoadEntityFromPacket(packet);
                planets[currPlayers[GetIndexOfId(clientId)].planet]->ReplaceEntity(index,e);
            }
        }else if (type == CSMessageType::UPDATE_PLAYER_DATA)
        {
            PlayerData& playerData = currPlayers[GetIndexOfId(clientId)];
            sf::Packet out;
            out << (uint16_t)CSMessageType::UPDATE_PLAYER_DATA;
            out << playerData.username;
            const uint8_t *data = static_cast<const uint8_t *>(packet.getData());
            std::size_t pos = packet.getReadPosition(); // just past the type you already read
            out.append(data + pos, packet.getDataSize() - pos);
            Broadcast(out,{clientId});
            uint64_t n;
            packet >> n;
            std::vector<uint8_t> data2;
            for (int i = 0; i < n; i ++)
            {
                uint8_t byte;
                packet >> byte;
                data2.push_back(byte);
            }
            Serializer s(Serializer::Mode::READ,Serializer::Format::BINARY,{},data2);
            playerData.Serialize(s);
        }
    }
    void Server::RegisterCurrentPlayers()
    {
        for (int i = 0; i < currPlayers.size(); i++)
        {
            for (int j = 0; j < allPlayers.size(); j++)
            {
                if (allPlayers[i].username == currPlayers[i].username)
                {
                    allPlayers[i] = currPlayers[i];
                }
            }
        }
    }
    void Server::SendJoinData(uint64_t clientId, sf::Packet &usernamePacket)
    {
        // REGISTERING PLAYER
        std::string username;
        usernamePacket >> username;
        BroadcastServerLog("Received username: " + username);
        PlayerData p;
        for (int i = 0; i < allPlayers.size(); i++)
        {
            if (allPlayers[i].username == username)
            {
                BroadcastServerLog("Player is existing. retrieving saved data");
                p = allPlayers[i];
                break;
            }
        }
        if (p.username == "")
        {
            BroadcastServerLog("New player registered");
            p.username = username;
            allPlayers.push_back(p);
        }
        currPlayers.push_back(p);

        // ASSEMBLING PACKET TO SEND BACK
        sf::Packet packet;
        packet << (uint16_t)CSMessageType::JOIN_DATA;
        // first bit of data is every other player currently in server
        // start by serializing the list of current players
        Serializer s(Serializer::Mode::WRITE, Serializer::Format::BINARY);
        int n = currPlayers.size();
        s.field("n", n);
        for (int i = 0; i < currPlayers.size(); i++)
        {
            s.field(std::to_string(i), currPlayers[i]);
        }
        auto data = s.binary();
        // put number of bytes in packet
        packet << data.size();
        // put bytes into packet
        packet.append(data.data(), data.size());
        // next, want to send all the chunks visible to the player
        sf::Vector2f targetResolution = p.resolution;
        int minX = floor((p.cameraPosition.x - targetResolution.x * p.cameraZoom / 2.f) / TILE_SIZE / CHUNK_SIZE);
        int maxX = ceil((p.cameraPosition.x + targetResolution.x * p.cameraZoom / 2.f) / TILE_SIZE / CHUNK_SIZE);
        int minY = floor((p.cameraPosition.y - targetResolution.y * p.cameraZoom / 2.f) / TILE_SIZE / CHUNK_SIZE);
        int maxY = ceil((p.cameraPosition.y + targetResolution.y * p.cameraZoom / 2.f) / TILE_SIZE / CHUNK_SIZE);
        // add number of chunks to packet
        packet << (int)((maxX - minX + 1) * (maxY - minY + 1));
        Planet *planet = planets[p.planet].get();
        for (int x = minX; x <= maxX; x++)
        {
            for (int y = minY; y <= maxY; y++)
            {
                if (!planet->chunks.contains({x, y}))
                {
                    planet->GenerateChunk({x, y});
                }
                Chunk *c = planet->chunks[{x, y}].get();
                // put chunk position into packet
                packet << c->position.x << c->position.y;
                auto b = c->GetByteData();
                // put size of bytes into packet
                packet << b.size();
                // put chunk data into packet
                packet.append(b.data(), b.size());
            }
        }
        //send entities
        uint64_t numEntities = (uint64_t)planet->entities.size();
        packet << numEntities;
        for (int i = 0; i < numEntities; i ++)
        {
            AppendEntityToPacket(packet,planet->entities[i].get());
        }

        SendToClient(clientId, packet);
        // let every other player know a new player has joined
        sf::Packet newPlayerPacket;
        newPlayerPacket << (uint16_t)CSMessageType::PLAYER_JOINED;
        Serializer s2(Serializer::Mode::WRITE, Serializer::Format::BINARY);
        s2.field("player", p);
        auto b = s2.binary();
        newPlayerPacket << (uint64_t)b.size();
        newPlayerPacket.append(b.data(), b.size());
        Broadcast(newPlayerPacket, {clientId});
    }
    void Server::BroadcastServerLog(std::string message)
    {
        sf::Packet p;
        p << (uint16_t)CSMessageType::SERVER_MESSAGE;
        p << message;
        Broadcast(p);
    }
    int Server::GetIndexOfId(uint64_t id)
    {
        for (int i = 0; i < clients.size(); i++)
        {
            if (clients[i].id == id)
            {
                return i;
            }
        }
        return -1;
    }
    void Server::BroadcastChatLog(std::string message, uint64_t clientId)
    {
        sf::Packet p;
        p << (uint16_t)CSMessageType::CHAT_MESSAGE;
        p << currPlayers[GetIndexOfId(clientId)].username;
        p << message;
        Broadcast(p, {clientId});
    }
    void Server::SendChunks(uint64_t clientId, std::vector<sf::Vector2i> &positions)
    {
        sf::Packet p;
        p << (uint16_t)CSMessageType::CHUNK_DATA;
        p << (uint64_t)positions.size();
        Planet *planet = planets[currPlayers[GetIndexOfId(clientId)].planet].get();
        for (int i = 0; i < positions.size(); i++)
        {
            if (!planet->chunks.contains(positions[i]))
            {
                planet->GenerateChunk(positions[i]);
            }
            Chunk *c = planet->chunks[positions[i]].get();
            // put chunk position into packet
            p << c->position.x << c->position.y;
            auto b = c->GetByteData();
            // put size of bytes into packet
            p << b.size();
            // put chunk data into packet
            p.append(b.data(), b.size());
        }
        SendToClient(clientId, p);
    }
}