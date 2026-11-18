#pragma once
#include "PlayerData.hpp"
#include "Planet.hpp"
#include "InputBundle.hpp"

namespace cc
{
    class World
    {
        public:
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
        private:
        void GenerateChunksInView(PlayerData* player);
    };
}
