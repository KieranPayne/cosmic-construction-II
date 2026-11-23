#pragma once
#include "Serializer.hpp"
#include "../PCH.hpp"
namespace cc
{
    class World;
    class Human;
    class Planet;
    enum class ActionType : uint16_t
    {
        NONE,
        DESTROY_TILE
    };
    struct ActionContext
    {
        Human* human;
        Planet* planet;
        World* world;
    };
    class Action
    {
        public:
        static Action* CreateActionFromType(ActionType type);
        ActionType type = ActionType::NONE;
        bool finished = false;
        ActionContext context;
        void SetPointers(Human* human);
        Action(){};
        virtual void Tick(){};
        virtual void Serialize(Serializer& s)
        {
            s.field("type",type);
            s.field("finished",finished); 
        }
    };
}