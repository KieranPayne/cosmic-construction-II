#pragma once
#include "../PCH.hpp"
#include "Planet.hpp"
#include <optional>

namespace cc
{
    struct PathResult
    {
        bool found = false;
        std::vector<sf::Vector2i> waypoints;
        /// The target tile coordinate reached (useful when searching by tile type).
        sf::Vector2i goalTile{0, 0};
    };

    class Pathfinder
    {
    public:
        static PathResult FindPath(
            Planet *planet,
            sf::Vector2f startPos,
            sf::Vector2i goalTile,
            bool allowAdjacent = false,
            int maxExpandedNodes = 4096);

        /**
         * @brief Finds a path from startPos to any loaded tile of targetTileType.
         * @param allowAdjacent If true, succeeds when reaching any walkable cardinal neighbor of a
         *                      matching tile (required when targetTileType is solid, e.g. trees/rocks).
         * @param maxExpandedNodes Safety cap on explored tiles before giving up.
         */
        static PathResult FindPathToTileType(
            Planet *planet,
            sf::Vector2f startPos,
            uint16_t targetTileType,
            bool allowAdjacent = true,
            int maxExpandedNodes = 4096);

        /**
         * @brief Finds the world tile coordinate of a loaded tile matching targetTileType near startPos
         *        without computing a path.
         * @param maxChunkRadius Maximum chunk ring distance around startPos to search.
         * @return The tile's world coordinate, or std::nullopt if none was found in range.
         */
        static std::optional<sf::Vector2i> FindTileOfType(
            Planet *planet,
            sf::Vector2f startPos,
            uint16_t targetTileType,
            int maxChunkRadius = 8);
    };
}