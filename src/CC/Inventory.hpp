#pragma once
#include "../PCH.hpp"
#include "ItemData.hpp"
namespace cc
{

    class Inventory
    {
        private:
        int numSlots;
        public:
        std::vector<ItemData> data;
        Inventory();
        void SetNumSlots(int numSlots);
        int GetNumSlots();
        void Serialize(Serializer& s);
    };
}