#pragma once
#include "../PCH.hpp"
#include "Planet.hpp"
namespace cc
{
    class Server
    {
        private:
        uint64_t seed;
        double timeSinceTick;
        public:
        struct ServerClient
        {
            sf::TcpSocket socket;
            uint64_t id;
        };
        sf::TcpListener listener;
        std::vector<ServerClient> clients;
        uint64_t currClientId = 0;
        uint64_t GetNextClientId();

        std::vector<std::unique_ptr<Planet>> planets;
        Server();
        int tps;
        void Tick();
        void Update(double dt);
        void SetSeed(uint64_t seed);
        uint64_t GetSeed();

        //networking functions
        void SendToClient(uint64_t clientId, sf::Packet& packet);
        void Broadcast(sf::Packet& packet, std::vector<uint64_t> exclusions = {});
        void AcceptClients();
        void ReceivePackets();
        void HandlePacket(uint64_t clientId, sf::Packet& packet);
    };
}