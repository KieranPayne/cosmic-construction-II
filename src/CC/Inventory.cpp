#include "Inventory.hpp"

namespace cc
{
    Inventory::Inventory()
    {
        numSlots = 0;
    }
    void Inventory::SetNumSlots(int numSlots)
    {
        while (this->numSlots < numSlots)
        {
            this->numSlots ++;
            data.push_back(ItemData());
        }
        while (this->numSlots > numSlots)
        {
            data.erase(data.end()-1);
            this->numSlots --;
        }
    }
    int Inventory::GetNumSlots()
    {
        return numSlots;
    }
    void Inventory::Serialize(Serializer& s)
    {
        s.field("numSlots",numSlots);
        s.field("data",data);
    }

}