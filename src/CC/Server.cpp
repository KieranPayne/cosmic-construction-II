#include "Server.hpp"
#include "CSMessage.hpp"
#include "Utils.hpp"
#include "InputBundle.hpp"
#include "SaveManager.hpp"
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
        world = std::make_unique<World>();
        world->server = this;
        // RegisterBuiltinCommands(commandSystem);
    }
    void Server::SetSeed(uint64_t seed)
    {
        world->SetSeed(seed);
    }
    void Server::Tick()
    {
        world->Tick();
    }
    uint64_t Server::GetSeed()
    {
        return world->seed;
    }
    void Server::Update(double dt)
    {
        AcceptClients();
        ReceivePackets();
        FlushOutgoing();
        timeSinceTick += dt;
        const double interval = 1.0 / TPS;
        timeSinceTick = std::min(timeSinceTick, interval * 5); // avoid a spiral after a long stall
        while (timeSinceTick >= interval)
        {
            BroadcastNextTick();

            Tick();
            timeSinceTick -= interval;
            if (world->currentTick % 60 == 0)
            {
                sf::Packet p;
                world->GenerateStateCheck(p);
                stateChecks.push_back({world->currentTick, p});
                std::vector<bool> clientChecks(clients.size(), false);
                for (int i = 0; i < clientChecks.size(); i++)
                {
                    if (clients[i].joinTick >= world->currentTick)
                    {
                        clientChecks[i] = true;
                    }
                }
                clientStateChecksVerified.push_back(clientChecks);
            }
        }
        // for (auto& p : planets)
        // {
        //     p->Update(dt);
        // }
    }
    uint64_t Server::GetNextClientId()
    {
        return currClientId++;
    }

    void Server::SendToClient(uint64_t clientId, sf::Packet &packet)
    {
        // TODO: if the recipient is the host's own client, process the packet directly instead of queueing it.
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
                // TODO: if this is the host's own client, process the packet directly instead of queueing it.
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

            client.socket.setBlocking(false);
            client.id = GetNextClientId();
            clients.push_back(std::move(client));
            world->players.push_back(PlayerData());
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
        PlayerData player = world->players[index];
        uint64_t id = clients[index].id;

        if (joined)
            world->SavePlayer(index);
        clients.erase(clients.begin() + index);
        world->players.erase(world->players.begin() + index);

        BroadcastServerLog("Client disconnected: " + std::to_string(id));
        if (joined)
        {
            sf::Packet p;
            p << (uint16_t)CSMessageType::PLAYER_LEFT;
            p << player.username;
            Broadcast(p);
        }
    }

    // void Server::SavePlayer(const PlayerData &p)
    // {
    //     for (auto &a : allPlayers)
    //         if (a.username == p.username)
    //         {
    //             a = p;
    //             return;
    //         }
    //     allPlayers.push_back(p);
    // }
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
        // else if (type == CSMessageType::CHAT_MESSAGE)
        // {
        //     std::string message;
        //     packet >> message;
        //     if (!commandSystem.TryExecute(*this, clientId, message))
        //     {
        //         BroadcastChatLog(message, clientId);
        //     }
        // }
        else if (type == CSMessageType::STATE_CHECK)
        {
            CompareStateChecks(clientId, packet);
        }
        else if (type == CSMessageType::CLIENT_INPUT_BUNDLE)
        {
            ProcessInputBundle(clientId, packet);
        }
        // else if (type == CSMessageType::REQUEST_UPDATE_KNOWLEDGE_DB)
        // {
        //     Serializer s = ReadSerializerFromPacket(packet);
        //     sf::Packet out;
        //     out << (uint16_t)CSMessageType::UPDATE_KNOWLEDGE_DB;
        //     AppendSerializerToPacket(out, s);
        //     Broadcast(out, {clientId});
        //     knowledgeDatabase.Serialize(s);
        //     if (SaveManager::debuggingKnowledgeGraph)
        //     {
        //         SaveKnowledgeDatabase(SaveManager::GetSavedataDir() + "/knowledge graph");
        //     }
        //     else
        //     {
        //         SaveKnowledgeDatabase(SaveManager::savePath + "/knowledge graph");
        //     }
        // }
    }
    void Server::ProcessInputBundle(uint64_t clientId, sf::Packet &p)
    {
        uint64_t tick;
        p >> tick;
        auto data = ReadBytesFromPacket(p);
        Serializer s(Serializer::Mode::READ, Serializer::Format::BINARY, {}, data);
        InputBundle inputBundle;
        inputBundle.Serialize(s);
        inputBundle.clientId = clientId;
        inputBundle.username = world->players[GetIndexOfId(clientId)].username;
        if (tick < world->currentTick)
        {
            tick = world->currentTick;
        }
        while (true)
        {
            // sequential inputs from the same client are spaced one tick apart
            bool exists = false;
            if (queuedInputs.contains(tick))
            {
                for (auto &in : queuedInputs[tick])
                {
                    if (in.clientId == clientId)
                    {
                        tick++;
                        exists = true;
                        break;
                    }
                }
            }
            if (!queuedInputs.contains(tick) || !exists)
            {
                queuedInputs[tick].push_back(std::move(inputBundle));
                break;
            }
        }
    }
    void Server::SendJoinData(uint64_t clientId, sf::Packet &usernamePacket)
    {
        ServerClient *client = GetClient(clientId);
        if (!client)
            return;
        // REGISTERING PLAYER
        client->joinTick = world->currentTick;
        std::string username;
        usernamePacket >> username;
        BroadcastServerLog("Received username: " + username);
        PlayerData p;
        for (int i = 0; i < world->playerHistory.size(); i++)
        {
            if (world->playerHistory[i].username == username)
            {
                BroadcastServerLog("Player is existing. retrieving saved data");
                p = world->playerHistory[i];
                break;
            }
        }
        if (p.username == "")
        {
            BroadcastServerLog("New player registered");
            p.username = username;
            world->playerHistory.push_back(p);
        }
        world->players[GetIndexOfId(clientId)] = p;
        // TODO: fix problem with client->player, need to make it sync up with the playerdata in World
        //  client->player = p;
        client->joined = true;

        // ASSEMBLING PACKET TO SEND BACK
        sf::Packet packet;
        packet << (uint16_t)CSMessageType::JOIN_DATA;
        world->AppendJoinData(packet);

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
        InputBundle in;
        in.chatMessages.push_back(ChatMessage(ChatMessageOrigin::SERVER, message));
        queuedInputs[world->currentTick].push_back(std::move(in));
        // sf::Packet p;
        // p << (uint16_t)CSMessageType::SERVER_MESSAGE;
        // p << message;
        // Broadcast(p);
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
    // void Server::BroadcastChatLog(std::string message, uint64_t clientId)
    // {
    //     InputBundle in;
    //     // sf::Packet p;
    //     // p << (uint16_t)CSMessageType::CHAT_MESSAGE;
    //     // p << world->players[GetIndexOfId(clientId)].username;
    //     // p << message;
    //     // Broadcast(p, {clientId});
    // }
    // void Server::SendChunks(uint64_t clientId, std::vector<sf::Vector2i> &positions)
    // {
    //     sf::Packet p;
    //     p << (uint16_t)CSMessageType::CHUNK_DATA;
    //     p << (uint64_t)positions.size();
    //     Planet *planet = planets[GetClient(clientId)->player.planet].get();
    //     for (int i = 0; i < positions.size(); i++)
    //     {
    //         if (!planet->chunks.contains(positions[i]))
    //         {
    //             planet->GenerateChunk(positions[i]);
    //         }
    //         Chunk *c = planet->chunks[positions[i]].get();
    //         // put chunk position into packet
    //         p << c->position.x << c->position.y;
    //         auto b = c->GetByteData();
    //         // put size of bytes into packet
    //         p << (uint64_t)b.size();
    //         // put chunk data into packet
    //         p.append(b.data(), b.size());
    //     }
    //     SendToClient(clientId, p);
    // }
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
                Update(dt);
            }
            catch (const std::exception &e)
            {
                std::cerr << "Server thread error: " << e.what() << '\n';
            }
        }
    }
    void Server::BroadcastNextTick()
    {
        sf::Packet p;
        p << (uint16_t)CSMessageType::BROADCAST_NEXT_TICK;
        p << world->currentTick;
        if (!queuedInputs.contains(world->currentTick))
        {
            p << (uint64_t)0;
            Broadcast(p);
            return;
        }
        std::vector<InputBundle> &bundles = queuedInputs[world->currentTick];

        p << (uint64_t)bundles.size();
        for (int i = 0; i < bundles.size(); i++)
        {
            Serializer s(Serializer::Mode::WRITE, Serializer::Format::BINARY);
            bundles[i].Serialize(s);
            auto data = s.binary();
            p << (uint64_t)data.size();
            p.append(data.data(), data.size());
        }
        Broadcast(p);
        for (int i = 0; i < bundles.size(); i++)
        {
            world->EnactInputBundle(bundles[i]);
        }
        queuedInputs.erase(world->currentTick);
    }

    void Server::CompareStateChecks(uint64_t clientId, sf::Packet &packet)
    {
        uint64_t tick;
        packet >> tick;

        for (size_t i = 0; i < stateChecks.size(); i++)
        {
            if (stateChecks[i].first == tick)
            {
                sf::Packet serverPacket(stateChecks[i].second);
                uint64_t serverTick;
                serverPacket >> serverTick;

                uint64_t serverKnowledgeChecksum;
                serverPacket >> serverKnowledgeChecksum;
                uint64_t clientKnowledgeChecksum;
                packet >> clientKnowledgeChecksum;
                if (serverKnowledgeChecksum != clientKnowledgeChecksum)
                {
                    std::cerr << "STATE DESYNCED; knowledge database checksum mismatch" << std::endl;
                    std::abort();
                }

                uint64_t clientNumPlanets;
                packet >> clientNumPlanets;
                uint64_t serverNumPlanets;
                serverPacket >> serverNumPlanets;
                if (clientNumPlanets != serverNumPlanets)
                {
                    std::cerr << "STATE DESYNCED; planet count mismatch" << std::endl;
                    std::abort();
                }

                for (uint64_t p = 0; p < serverNumPlanets; p++)
                {
                    uint64_t serverSeed, clientSeed;
                    serverPacket >> serverSeed;
                    packet >> clientSeed;
                    if (serverSeed != clientSeed)
                    {
                        std::cerr << "STATE DESYNCED; planet " << p << " seed mismatch" << std::endl;
                        std::abort();
                    }

                    uint64_t serverGenChecksum, clientGenChecksum;
                    serverPacket >> serverGenChecksum;
                    packet >> clientGenChecksum;
                    if (serverGenChecksum != clientGenChecksum)
                    {
                        std::cerr << "STATE DESYNCED; planet " << p << " generator checksum mismatch" << std::endl;
                        std::abort();
                    }

                    uint64_t serverNumChunks;
                    serverPacket >> serverNumChunks;
                    uint64_t clientNumChunks;
                    packet >> clientNumChunks;
                    if (serverNumChunks != clientNumChunks)
                    {
                        std::cerr << "STATE DESYNCED; chunk count mismatch" << std::endl;
                        std::abort();
                    }

                    for (uint64_t j = 0; j < serverNumChunks; j++)
                    {
                        sf::Vector2i serverPos;
                        serverPacket >> serverPos.x >> serverPos.y;
                        sf::Vector2i clientPos;
                        packet >> clientPos.x >> clientPos.y;
                        if (clientPos != serverPos)
                        {
                            std::cerr << "STATE DESYNCED; chunk position mismatch" << std::endl;
                            std::abort();
                        }

                        uint64_t serverChecksum;
                        serverPacket >> serverChecksum;
                        uint64_t clientChecksum;
                        packet >> clientChecksum;
                        if (serverChecksum != clientChecksum)
                        {
                            std::cerr << "STATE DESYNCED; chunk checksum mismatch" << std::endl;
                            std::abort();
                        }
                    }

                    uint64_t serverNumEntities;
                    serverPacket >> serverNumEntities;
                    uint64_t clientNumEntities;
                    packet >> clientNumEntities;
                    if (serverNumEntities != clientNumEntities)
                    {
                        std::cerr << "STATE DESYNCED; entity count mismatch" << std::endl;
                        std::abort();
                    }

                    for (uint64_t j = 0; j < serverNumEntities; j++)
                    {
                        uint64_t serverChecksum;
                        serverPacket >> serverChecksum;
                        uint64_t clientChecksum;
                        packet >> clientChecksum;
                        if (serverChecksum != clientChecksum)
                        {
                            std::cerr << "STATE DESYNCED; entity checksum mismatch" << std::endl;
                            std::abort();
                        }
                    }
                }

                int index = GetIndexOfId(clientId);
                if (index >= 0 && static_cast<size_t>(index) < clientStateChecksVerified[i].size())
                {
                    clientStateChecksVerified[i][index] = true;
                }

                // Check all clients have sent state check
                if (std::find(clientStateChecksVerified[i].begin(), clientStateChecksVerified[i].end(), false) == clientStateChecksVerified[i].end())
                {
                    stateChecks.erase(stateChecks.begin() + i);
                    clientStateChecksVerified.erase(clientStateChecksVerified.begin() + i);
                }
                break;
            }
        }
    }

    bool Server::IsHost(uint64_t clientId)
    {
        ServerClient *client = GetClient(clientId);
        if (!client)
            return false;
        auto address = client->socket.getRemoteAddress();
        return address.has_value() && *address == sf::IpAddress::getLocalAddress();
    }

    // void Server::BroadcastAddedEntity(Planet *planet, Entity *entity)
    // {
    //     // The planet's own `index` isn't reliable on the server, so find it by pointer.
    //     int planetIndex = -1;
    //     for (int i = 0; i < (int)planets.size(); i++)
    //     {
    //         if (planets[i].get() == planet)
    //         {
    //             planetIndex = i;
    //             break;
    //         }
    //     }
    //     if (planetIndex == -1)
    //         return;

    //     sf::Packet p;
    //     p << (uint16_t)CSMessageType::ADD_ENTITIES;
    //     p << (int)1;
    //     AppendEntityToPacket(p, entity);
    //     for (auto &c : clients)
    //     {
    //         if (c.joined && c.player.planet == planetIndex)
    //             c.outgoing.push_back(p);
    //     }
    // }
}