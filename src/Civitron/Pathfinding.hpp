#pragma once
#include "../PCH.hpp"
#include "Chunk.hpp"
#include "Planet.hpp"
namespace Civitron{
    namespace Pathfinding{
        struct PathfindSettings{
            bool allowNeighbour = false;
            int maxDist = 1;
        };
        extern int WEIGHTS[3][3][3];
        typedef std::unordered_map<sf::Vector3i,sf::Vector3i,ChunkHash> NodeTable;
        typedef std::unordered_map<sf::Vector3i, float, ChunkHash> ScoreTable;
        typedef std::unordered_map<sf::Vector3i, bool, ChunkHash> SearchedTable;
        //A Star heuristic
        float Heuristic(sf::Vector3i& cPos, sf::Vector3i& goalPos);
        std::vector<sf::Vector3i> ReconstructPath(NodeTable& cameFrom, sf::Vector3i current);
        int SearchFScore(std::vector<float> arr, int start, int end, float score);
        std::vector<sf::Vector3i> PathFind(sf::Vector3i startPos, sf::Vector3i goalPos, Planet* p, PathfindSettings settings);
        //second element states whether search was successful
        std::pair<sf::Vector3i, bool> SearchForTile(sf::Vector3i startPos, Tile tile, Planet* p, PathfindSettings settings);
    }
}


//TODO: create parameters struct and add settle for neighbours parameter
//then adjust both search and pathfind to use this