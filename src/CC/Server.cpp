#include "Server.hpp"
#include "CSMessage.hpp"
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
            std::cout << "ServerClient connected: " << clients.back().id << '\n';
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
                std::cout << "ServerClient disconnected: "
                          << clients[i].id << '\n';

                clients.erase(clients.begin() + i);
            }
        }
    }
    void Server::HandlePacket(std::uint64_t clientId, sf::Packet &packet)
    {
        std::cout << "SERVER: Handling packet" << std::endl;
        uint16_t t;
        packet >> t;
        CSMessageType type = (CSMessageType)t;
        if (type == CSMessageType::SEND_USERNAME)
        {
            SendJoinData(clientId,packet);
        }
    }
    void Server::RegisterCurrentPlayers()
    {
        for (int i = 0; i < currPlayers.size(); i ++)
        {
            for (int j = 0; j < allPlayers.size(); j ++)
            {
                if (allPlayers[i].username == currPlayers[i].username)
                {
                    allPlayers[i] = currPlayers[i];
                }
            }
        }
    }
    void Server::SendJoinData(uint64_t clientId, sf::Packet& usernamePacket)
    {
        //REGISTERING PLAYER
        std::string username;
        usernamePacket >> username;
        std::cout << "SERVER: Received username: " << username << std::endl;
        PlayerData p;
        for (int i = 0; i < allPlayers.size(); i ++)
        {
            if (allPlayers[i].username == username)
            {
                p = allPlayers[i];
                break;
            }
        }
        if (p.username == "")
        {
            std::cout << "SERVER: new player registered" << std::endl;
            p.username = username;
            allPlayers.push_back(p);
        }
        currPlayers.push_back(p);
        
        //ASSEMBLING PACKET TO SEND BACK
        sf::Packet packet;
        packet << (uint16_t)CSMessageType::JOIN_DATA;
        //first bit of data is every other player currently in server
        //start by serializing the list of current players
        Serializer s(Serializer::Mode::WRITE,Serializer::Format::BINARY);
        int n = currPlayers.size();
        s.field("n",n);
        for (int i = 0; i < currPlayers.size(); i ++)
        {
            s.field(std::to_string(i),currPlayers[i]);
        }
        auto data = s.binary();
        //put number of bytes in packet
        packet << data.size();
        //put bytes into packet
        packet.append(data.data(),data.size());
        //next, want to send all the chunks visible to the player
        sf::Vector2f targetResolution {3840.f,2160.f};
        int minX = floor((p.cameraPosition.x - targetResolution.x * p.cameraZoom / 2.f) / TILE_SIZE / CHUNK_SIZE);
        int maxX = ceil((p.cameraPosition.x + targetResolution.x * p.cameraZoom / 2.f) / TILE_SIZE / CHUNK_SIZE);
        int minY = floor((p.cameraPosition.y - targetResolution.y * p.cameraZoom / 2.f) / TILE_SIZE / CHUNK_SIZE);
        int maxY = ceil((p.cameraPosition.y + targetResolution.y * p.cameraZoom / 2.f) / TILE_SIZE / CHUNK_SIZE);
        //add number of chunks to packet
        packet << (int)((maxX- minX + 1) * (maxY - minY + 1));
        Planet* planet = planets[p.planet].get();
        for (int x = minX; x <= maxX; x ++)
        {
            for (int y = minY; y <= maxY; y ++)
            {
                if (!planet->chunks.contains({x,y}))
                {
                    planet->GenerateChunk({x,y});
                }
                Chunk* c = planet->chunks[{x,y}].get();
                //put chunk position into packet
                packet << c->position.x << c->position.y;
                auto b = c->GetByteData();
                //put size of bytes into packet
                packet << b.size();
                //put chunk data into packet
                packet.append(b.data(),b.size());
            }
        }        
        SendToClient(clientId,packet);
        //let every other player know a new player has joined
        sf::Packet newPlayerPacket;
        newPlayerPacket << (uint16_t)CSMessageType::PLAYER_JOINED;
        Serializer s2(Serializer::Mode::WRITE,Serializer::Format::BINARY);
        s2.field("player",p);
        auto b = s2.binary();
        newPlayerPacket << (uint64_t)b.size();
        newPlayerPacket.append(b.data(),b.size());
        Broadcast(newPlayerPacket,{clientId});
    }
}