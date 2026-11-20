#pragma once
#include "../PCH.hpp"
namespace cc
{
    class World;
    class Human;
    class Planet;
    struct ActionContext
    {
        Human* human;
        Planet* planet;
        World* world;
    };
    class Action
    {
        ActionContext context;
        virtual void Tick(){};
    };
}