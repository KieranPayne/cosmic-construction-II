#pragma once
#include "../PCH.hpp"
#include "Planet.hpp"
#include "PlayerData.hpp"
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
            std::unordered_set<sf::Vector2i, ChunkHash> sentChunks;
        };
        sf::TcpListener listener;
        //every player that has joined this save
        std::vector<PlayerData> allPlayers = {};
        std::vector<PlayerData> currPlayers = {};

        std::vector<ServerClient> clients;
        uint64_t currClientId = 0;
        uint64_t GetNextClientId();

        std::vector<std::unique_ptr<Planet>> planets = {};
        Server();
        int tps;
        void Tick();
        void Update(double dt);
        void SetSeed(uint64_t seed);
        uint64_t GetSeed();

        //networking functions
        void Start(unsigned short port);
        void SendToClient(uint64_t clientId, sf::Packet& packet);
        void Broadcast(sf::Packet& packet, std::vector<uint64_t> exclusions = {});
        void AcceptClients();
        void ReceivePackets();
        void HandlePacket(uint64_t clientId, sf::Packet& packet);
        void SendJoinData(uint64_t clientId, sf::Packet& usernamePacket);
        //enters up to date information on all current players in to the all players list
        void RegisterCurrentPlayers();
        void BroadcastServerLog(std::string message);
        void BroadcastChatLog(std::string message, uint64_t clientId);
        int GetIndexOfId(uint64_t id);
        void SendChunks(uint64_t clientId, std::vector<sf::Vector2i>& positions);
    };
}