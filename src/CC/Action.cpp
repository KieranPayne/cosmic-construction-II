#include "Action.hpp"
#include "Human.hpp"
#include "ActionDestroyTile.hpp"
#include "Planet.hpp"
#include "ActionPathfind.hpp"
namespace cc
{
    Action* Action::CreateActionFromType(ActionType type)
    {
        switch (type)
        {
            case ActionType::NONE:
                return new Action();
            case ActionType::DESTROY_TILE:
                return new ActionDestroyTile();
            case ActionType::PATHFIND:
                return new ActionPathfind();
        }
    }
    void Action::SetPointers(Human* human)
    {
        context.human = human;
        context.planet = human->planet;
        context.world = human->planet->world;
    }
}