#include "ActionPathfind.hpp"
#include "Pathfinder.hpp"
#include "Human.hpp"
namespace cc
{
    ActionPathfind::ActionPathfind()
    {
        type = ActionType::PATHFIND;
    }
    void ActionPathfind::SetGoal(sf::Vector2i tilePos, bool calculatePath)
    {
        goalPos = tilePos;
        if (!calculatePath)
        {
            return;
        }
        auto pathResult = Pathfinder::FindPath(context.planet,context.human->position,tilePos,true);
        if (!pathResult.found)
        {
            //wait 1 seconds before trying to find a path again
            tryPathfindCooldown = 60;
            return;
        }
        humanStartPos = context.human->position;
        path = std::move(pathResult.waypoints);
        pathIndex = 0;
        ticksSinceMoved = 0;
    }
    void ActionPathfind::Tick()
    {
        if (path.size() > 0)
        {
            sf::Vector2f posA;
            sf::Vector2f posB;
            if (pathIndex == 0)
            {
                posA = humanStartPos;
                posB = (sf::Vector2f)path[0];
            }else
            {
                posA = (sf::Vector2f)path[pathIndex-1];
                posB = (sf::Vector2f)path[pathIndex];
            }
            context.human->position = Lerp(posA,posB,(float)ticksSinceMoved / ticksPerTile);
            ticksSinceMoved ++;
            if (ticksSinceMoved > ticksPerTile)
            {
                ticksSinceMoved = 0;
                pathIndex ++;
                if (pathIndex > path.size() - 1)
                {
                    finished = true;
                }
            }
        }else
        {
            tryPathfindCooldown --;
            if (tryPathfindCooldown <= 0)
            {
                SetGoal(goalPos);
            }
        }
    }
    void ActionPathfind::Serialize(Serializer& s)
    {
        Action::Serialize(s);
        s.field("path",path);
        s.field("pathIndex",pathIndex);
        s.field("ticksPerTile",ticksPerTile);
        s.field("ticksSincedMoved",ticksSinceMoved);
        s.field("goalPos",goalPos);
        s.field("tryPathfindCooldown",tryPathfindCooldown);
        s.field("humanStartPos",humanStartPos);
    }
}