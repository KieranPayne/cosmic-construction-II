#pragma once
#include "Action.hpp"
namespace cc
{
    class ActionPathfind : public Action
    {
        public:
        std::vector<sf::Vector2i> path;
        int pathIndex = 0;
        int ticksPerTile = 20;
        ActionPathfind();
        void SetGoal(sf::Vector2i tilePos, bool calculatePath = true);
        void Tick();
        void Serialize(Serializer& s);
        private:
        int ticksSinceMoved = 0;
        // bool stepPath();
        sf::Vector2i goalPos = sf::Vector2i(0,0);
        int tryPathfindCooldown = 0;
        sf::Vector2f humanStartPos = sf::Vector2f(0,0);
    };
}