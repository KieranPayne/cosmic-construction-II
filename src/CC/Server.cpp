#include "Server.hpp"

namespace cc
{
    Server::Server()
    {
		planets.push_back(std::make_unique<Planet>());
    }
    void Server::SetSeed(uint64_t seed)
    {
        this->seed = seed;
        for (int i = 0; i < planets.size(); i ++)
        {
            planets[i]->SetSeed(seed + i);
        }
    }
    void Server::Tick()
    {
        for (auto& p : planets)
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
        timeSinceTick += dt;
        if (timeSinceTick > (1.0 / tps))
        {
            Tick();
            timeSinceTick -= dt;
        }
    }
}