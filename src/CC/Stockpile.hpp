#pragma once
#include "TileEntity.hpp"
#include "TileInfo.hpp"
namespace cc
{
    class Stockpile : public TileEntity
    {
        public:
        Stockpile()
        {
            static int myType = TileInfo::nameLookup["Stockpile"]->id;
            type = myType;
        }
        void GetVertices(sf::VertexArray& arr, int& index);

    };
}