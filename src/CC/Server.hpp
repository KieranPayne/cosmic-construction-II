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
        std::vector<std::unique_ptr<Planet>> planets;
        Server();
        int tps;
        void Tick();
        void Update(double dt);
        void SetSeed(uint64_t seed);
        uint64_t GetSeed();
    };
}