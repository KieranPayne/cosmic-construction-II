#pragma once
#include "PlayerData.hpp"
#include "Planet.hpp"
#include "InputBundle.hpp"
#include "CommandSystem.hpp"
#include "KnowledgeSystem.hpp"
#include "KnowledgeDatabase.hpp"
namespace cc
{
    class Client;
    class Server;
    class World
    {
        public:
        CommandSystem commandSystem;
        KnowledgeDatabase knowledgeDatabase;
        KnowledgeSystem knowledgeSystem{knowledgeDatabase};
        Client* client = nullptr;
        Server* server = nullptr;
        std::vector<std::unique_ptr<Planet>> planets;
        uint64_t currentTick = 0;
        //all players that have ever been on the save
        std::vector<PlayerData> playerHistory;
        std::vector<PlayerData> players;
        uint64_t seed;

        World();
        void Tick();
        void EnactInputBundle(InputBundle& bundle);
        void GenerateStateCheck(sf::Packet& packet);
        void AppendJoinData(sf::Packet& packet);
        void LoadJoinData(sf::Packet& packet);
        void SavePlayer(int index);
        void Serialize(Serializer& s);
        void Save();
        void Load();
        void SetSeed(uint64_t seed);
        void AddPlayer(PlayerData player);
        void ShowMessageToClient(std::string senderName, std::string message);
        // pass path without file extension pls
        void LoadKnowledgeDatabase(std::string path);
        // pass path without file extension pls
        void SaveKnowledgeDatabase(std::string path);
        private:
        void GenerateChunksInView(PlayerData* player);
        //shows a secret message just to the client (does nothing if the world belongs to the server)
    };
}
