#include "TileEntity.hpp"
#include "TileInfo.hpp"
#include "Stockpile.hpp"
namespace cc
{
    TileEntity* CreateTileEntityFromType(uint16_t type)
    {
        static int stockpileType = TileInfo::nameLookup["Stockpile"]->id; 
        if (type == 0)
        {
            return new TileEntity();
        }else if (type == stockpileType)
        {
            return new Stockpile();
        }
        return nullptr;
    }
}