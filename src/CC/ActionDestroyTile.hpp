#pragma once
#include "Action.hpp"

namespace cc
{
    class ActionDestroyTile : public Action
    {
        public:
        //0-up 1-right 2-down 3-left
        int direction = 0;
        //ticks since started destroying
        float progress = 0;
        ActionDestroyTile();
        void Tick();
        void Serialize(Serializer& s);
        private:
        //destroy time of target (saved to avoid rechecking)
        float destroyTime = -1;
    };
}