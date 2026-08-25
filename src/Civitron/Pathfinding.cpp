#include "Pathfinding.hpp"
#include "TileInfo.hpp"
#include <queue>
namespace Civitron
{
    // namespace Pathfinding
    // {
    //     int WEIGHTS[3][3][3] = {
    //         {{7, 6, 7}, {2, 1, 2}, {7, 6, 7}},
    //         {{3, 2, 3}, {1, 0, 1}, {3, 2, 3}},
    //         {{7, 6, 7}, {2, 1, 2}, {7, 6, 7}}};
           
    //     bool CloserThanMax(sf::Vector3i start, sf::Vector3i end, int max)
    //     {
    //         return abs(end.x - start.x) <= max &&
    //                abs(end.y - start.y) <= max &&
    //                abs(end.z - start.z) <= max;
    //     }
    //     // A Star heuristic
    //     float Heuristic(sf::Vector3i &cPos, sf::Vector3i &goalPos)
    //     {
    //         return (goalPos - cPos).lengthSquared();
    //     }

    //     std::vector<sf::Vector3i> ReconstructPath(NodeTable &cameFrom, sf::Vector3i current)
    //     {
    //         std::vector<sf::Vector3i> totalPath = {current};
    //         while (cameFrom.contains(current))
    //         {
    //             current = cameFrom[current];
    //             totalPath.push_back(current);
    //         }
    //         std::reverse(totalPath.begin(), totalPath.end());
    //         return totalPath;
    //     }
    //     int SearchFScore(std::vector<float> arr, int start, int end, float score)
    //     {
    //         if (start == end)
    //         {
    //             return arr[start] >= score ? start : -1;
    //         }
    //         int mid = start + (end - start) / 2;
    //         if (score > arr[mid])
    //         {
    //             return SearchFScore(arr, start, mid, score);
    //         }
    //         int ret = SearchFScore(arr, mid + 1, end, score);
    //         return ret == -1 ? mid : ret;
    //     }
    //     std::vector<sf::Vector3i> PathFind(sf::Vector3i startPos, sf::Vector3i goalPos, Planet *p, PathfindSettings settings)
    //     {
    //         std::vector<sf::Vector3i> openSet = {startPos};
    //         std::vector<float> fScores = {1e7f};
    //         NodeTable cameFrom;
    //         ScoreTable gScore;
    //         gScore[startPos] = 0;
    //         while (openSet.size() > 0)
    //         {
    //             sf::Vector3i currentNode = openSet.back();

    //             openSet.pop_back();
    //             fScores.pop_back();
    //             if (!CloserThanMax(startPos, currentNode, settings.maxDist))
    //             {
    //                 continue;
    //             }
    //             bool walkable = p->TileIsWalkable(currentNode);
    //             if (currentNode == goalPos)
    //             {
    //                 if (settings.allowNeighbour || walkable)
    //                 {
    //                     return ReconstructPath(cameFrom, currentNode);
    //                 }
    //             }
    //             if (!walkable)
    //             {
    //                 continue;
    //             }
    //             int numNeighbours = 0;
    //             std::array<sf::Vector3i, 26> neighbours;
    //             std::array<float, 26> weights;
    //             for (int x = -1; x <= 1; x++)
    //             {
    //                 for (int z = -1; z <= 1; z++)
    //                 {
    //                     for (int y = -1; y <= 1; y++)
    //                     {

    //                         sf::Vector3i offset = {x, y, z};
    //                         sf::Vector3i pos = currentNode + offset;
    //                         if (!cameFrom.contains(pos) && !(x == 0 && y == 0 && z == 0))
    //                         {
    //                             neighbours[numNeighbours] = pos;
    //                             weights[numNeighbours] = (float)WEIGHTS[offset.x + 1][offset.y + 1][offset.z + 1];
    //                             numNeighbours++;
    //                         }
    //                     }
    //                 }
    //             }
    //             for (int i = 0; i < numNeighbours; i++)
    //             {
    //                 float tentGScore = (float)(gScore.contains(currentNode) ? gScore[currentNode] : 1e7f) + weights[i];
    //                 if (tentGScore < (float)(gScore.contains(neighbours[i]) ? gScore[neighbours[i]] : 1e7f))
    //                 {
    //                     cameFrom[neighbours[i]] = currentNode;
    //                     gScore[neighbours[i]] = tentGScore;
    //                     float fscore = tentGScore + Heuristic(neighbours[i], goalPos);
    //                     if (std::find(openSet.begin(), openSet.end(), neighbours[i]) == openSet.end())
    //                     {
    //                         if (openSet.size() == 0)
    //                         {
    //                             openSet.push_back(neighbours[i]);
    //                             fScores.push_back(fscore);
    //                         }
    //                         int index = SearchFScore(fScores, 0, fScores.size() - 1, fscore) + 1;
    //                         openSet.insert(openSet.begin() + index, neighbours[i]);
    //                         fScores.insert(fScores.begin() + index, fscore);
    //                     }
    //                 }
    //             }
    //         }
    //         return {};
    //     }
        
    //     std::pair<sf::Vector3i, bool> SearchForTile(sf::Vector3i startPos, Tile tile, Planet *p, PathfindSettings settings)
    //     {
    //         std::queue<sf::Vector3i> openSet;
    //         openSet.push(startPos);
    //         SearchedTable searched;
    //         while (openSet.size() > 0)
    //         {
    //             sf::Vector3i node = openSet.front();
    //             openSet.pop();
    //             if ( !CloserThanMax(startPos,node,settings.maxDist))
    //             {
    //                 continue;
    //             }
    //             if (p->GetTileAt(node)->type == tile.type && (settings.allowNeighbour || p->TileIsWalkable(node)))
    //             {
    //                 return {node, true};
    //             }
    //             if (!p->TileIsWalkable(node))
    //             {
    //                 continue;
    //             }
    //             for (int x = -1; x < 2; x++)
    //             {
    //                 for (int z = -1; z < 2; z++)
    //                 {
    //                     for (int y = -1; y < 2; y ++){
    //                         sf::Vector3i pos = node + sf::Vector3i(x, y, z);
    //                         if (searched.contains(pos)){
    //                             continue;
    //                         }
    //                         openSet.push(pos);
    //                         searched[pos] = true;
    //                     }
    //                 }
    //             }
    //         }
    //         return {sf::Vector3i(0, 0, 0), false};
    //     }
    // }

}