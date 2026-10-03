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
        FlushOutgoing();
        timeSinceTick += dt;
        const double interval = 1.0 / tps;
        timeSinceTick = std::min(timeSinceTick, interval * 5); // avoid a spiral after a long stall
        while (timeSinceTick >= interval)
        {
            Tick();
            timeSinceTick -= interval;
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
                c.outgoing.push_back(packet);
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
                client.outgoing.push_back(packet);
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
            bool disconnected = false;
            while (true) // drain everything waiting, not just one packet
            {
                sf::Packet packet;
                auto status = clients[i].socket.receive(packet);
                if (status == sf::Socket::Status::Done)
                    HandlePacket(clients[i].id, packet);
                else if (status == sf::Socket::Status::NotReady)
                    break;
                else
                {
                    disconnected = true;
                    break;
                }
            }
            if (disconnected)
                RemoveClient(i);
            else
                ++i;
        }
    }

    void Server::RemoveClient(std::size_t index)
    {
        bool joined = clients[index].joined;
        PlayerData player = clients[index].player;
        uint64_t id = clients[index].id;

        if (joined)
            SavePlayer(player);
        clients.erase(clients.begin() + index);

        BroadcastServerLog("Client disconnected: " + std::to_string(id));
        if (joined)
        {
            sf::Packet p;
            p << (uint16_t)CSMessageType::PLAYER_LEFT;
            p << player.username;
            Broadcast(p);
        }
    }

    void Server::SavePlayer(const PlayerData &p)
    {
        for (auto &a : allPlayers)
            if (a.username == p.username)
            {
                a = p;
                return;
            }
        allPlayers.push_back(p);
    }
    void Server::HandlePacket(std::uint64_t clientId, sf::Packet &packet)
    {
        uint16_t t;
        packet >> t;
        CSMessageType type = (CSMessageType)t;
        ServerClient *client = GetClient(clientId);
        if (!client)
            return;
        if (type == CSMessageType::SEND_USERNAME ? client->joined : !client->joined)
            return;
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
            int planetIndex = GetClient(clientId)->player.planet;
            for (int i = 0; i < n; i++)
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
                planets[GetClient(clientId)->player.planet]->SetTileAt(position, Tile(tileType), e);
            }

            for (auto &c : clients)
                if (c.joined && c.player.planet == planetIndex && c.id != clientId)
                    c.outgoing.push_back(out);
        }
        else if (type == CSMessageType::REQUEST_ADD_ENTITIES)
        {
            sf::Packet out;
            out << (uint16_t)CSMessageType::ADD_ENTITIES;
            const uint8_t *data = static_cast<const uint8_t *>(packet.getData());
            std::size_t pos = packet.getReadPosition(); // just past the type you already read
            out.append(data + pos, packet.getDataSize() - pos);
            int planetIndex = GetClient(clientId)->player.planet;
            for (auto &c : clients)
                if (c.joined && c.player.planet == planetIndex && c.id != clientId)
                    c.outgoing.push_back(out);
            int n;
            packet >> n;
            for (int i = 0; i < n; i++)
            {
                Entity *e = LoadEntityFromPacket(packet);
                planets[GetClient(clientId)->player.planet]->AddEntity(e, true);
            }
        }
        else if (type == CSMessageType::REQUEST_UPDATE_ENTITIES)
        {
            sf::Packet out;
            out << (uint16_t)CSMessageType::UPDATE_ENTITIES;
            const uint8_t *data = static_cast<const uint8_t *>(packet.getData());
            std::size_t pos = packet.getReadPosition(); // just past the type you already read
            out.append(data + pos, packet.getDataSize() - pos);
            int planetIndex = GetClient(clientId)->player.planet;
            for (auto &c : clients)
                if (c.joined && c.player.planet == planetIndex && c.id != clientId)
                    c.outgoing.push_back(out);
            int n;
            packet >> n;
            for (int i = 0; i < n; i++)
            {
                int index;
                packet >> index;
                Entity *e = LoadEntityFromPacket(packet);
                planets[GetClient(clientId)->player.planet]->ReplaceEntity(index, e);
            }
        }
        else if (type == CSMessageType::UPDATE_PLAYER_DATA)
        {
            PlayerData &playerData = GetClient(clientId)->player;
            sf::Packet out;
            out << (uint16_t)CSMessageType::UPDATE_PLAYER_DATA;
            out << playerData.username;
            const uint8_t *data = static_cast<const uint8_t *>(packet.getData());
            std::size_t pos = packet.getReadPosition(); // just past the type you already read
            out.append(data + pos, packet.getDataSize() - pos);
            Broadcast(out, {clientId});
            uint64_t n;
            packet >> n;
            std::vector<uint8_t> data2;
            for (int i = 0; i < n; i++)
            {
                uint8_t byte;
                packet >> byte;
                data2.push_back(byte);
            }
            Serializer s(Serializer::Mode::READ, Serializer::Format::BINARY, {}, data2);
            std::string name = playerData.username;
            playerData.Serialize(s);
            playerData.username = name;
            if (playerData.planet < 0 || playerData.planet >= (int)planets.size())
                playerData.planet = 0;
        }
    }

    void Server::SendJoinData(uint64_t clientId, sf::Packet &usernamePacket)
    {
        ServerClient *client = GetClient(clientId);
        if (!client)
            return;
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

        client->player = p;
        client->joined = true;

        // ASSEMBLING PACKET TO SEND BACK
        sf::Packet packet;
        packet << (uint16_t)CSMessageType::JOIN_DATA;
        // first bit of data is every other player currently in server
        // start by serializing the list of current players
        Serializer s(Serializer::Mode::WRITE, Serializer::Format::BINARY);
        std::vector<PlayerData *> joinedPlayers;
        for (auto &c : clients)
            if (c.joined)
                joinedPlayers.push_back(&c.player);
        int n = joinedPlayers.size();
        s.field("n", n);
        for (int i = 0; i < n; i++)
            s.field(std::to_string(i), *joinedPlayers[i]);
        auto data = s.binary();
        // put number of bytes in packet
        packet << (uint64_t)data.size();
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
                packet << (uint64_t)b.size();
                // put chunk data into packet
                packet.append(b.data(), b.size());
            }
        }
        // send entities
        uint64_t numEntities = (uint64_t)planet->entities.size();
        packet << numEntities;
        for (int i = 0; i < numEntities; i++)
        {
            AppendEntityToPacket(packet, planet->entities[i].get());
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
        p << GetClient(clientId)->player.username;
        p << message;
        Broadcast(p, {clientId});
    }
    void Server::SendChunks(uint64_t clientId, std::vector<sf::Vector2i> &positions)
    {
        sf::Packet p;
        p << (uint16_t)CSMessageType::CHUNK_DATA;
        p << (uint64_t)positions.size();
        Planet *planet = planets[GetClient(clientId)->player.planet].get();
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
            p << (uint64_t)b.size();
            // put chunk data into packet
            p.append(b.data(), b.size());
        }
        SendToClient(clientId, p);
    }
    void Server::FlushOutgoing()
    {
        for (auto &c : clients)
        {
            while (!c.outgoing.empty())
            {
                auto status = c.socket.send(c.outgoing.front());
                if (status == sf::Socket::Status::Done)
                    c.outgoing.pop_front();
                else
                    break; // Partial/NotReady: retry the SAME packet next frame
                           // (Disconnected/Error get caught by ReceivePackets)
            }
        }
    }
    Server::ServerClient *Server::GetClient(uint64_t id)
    {
        for (auto &c : clients)
            if (c.id == id)
                return &c;
        return nullptr;
    }
    bool Server::StartThread()
    {
        if (running.exchange(true))
            return false; // already running
        thread = std::thread(&Server::Run, this);
        return true;
    }

    void Server::Stop()
    {
        if (!running.exchange(false))
            return;
        if (thread.joinable())
            thread.join();
    }

    Server::~Server() { Stop(); }

    void Server::Run()
    {
        sf::Clock clock;
        while (running)
        {
            // Sleep until a socket has data (or a short timeout) instead of busy-looping.
            // Only this thread mutates `clients`, so no lock is needed to build the selector.
            sf::SocketSelector selector;
            selector.add(listener);
            bool backlog = false;
            for (auto &c : clients)
            {
                selector.add(c.socket);
                backlog |= !c.outgoing.empty(); // partial sends need a quick retry
            }
            selector.wait(backlog ? sf::milliseconds(1) : sf::milliseconds(5));

            double dt = clock.restart().asSeconds();
            try
            {
                std::lock_guard lock(mutex);
                Update(dt); // your existing Update, unchanged
            }
            catch (const std::exception &e)
            {
                std::cerr << "Server thread error: " << e.what() << '\n';
            }
        }
    }
}