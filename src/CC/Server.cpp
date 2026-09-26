#include "Server.hpp"

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

            // sf::Packet packet;
            // packet << (uint8_t)CSMessageType::JOIN_DATA;
            // AddNewPlayer(packet);
            client.id = GetNextClientId();
            clients.push_back(std::move(client));
            // clients.back().id = players.back().id;
            std::cout << "ServerClient connected: " << clients.back().id << '\n';
            // Tell the new client its ID
            // packet << static_cast<std::uint32_t>(clients.back().id);

            // clients.back().socket.send(packet);
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
        std::cout << "Handling packet" << std::endl;
    }
}