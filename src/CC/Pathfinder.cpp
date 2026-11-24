#include "Pathfinder.hpp"
#include "Chunk.hpp"
#include "TileInfo.hpp"
#include "Utils.hpp"
#include <algorithm>
#include <cmath>
#include <queue>
#include <unordered_map>

namespace cc
{
    namespace
    {
        struct NodeRecord
        {
            sf::Vector2i parent{0, 0};
            int gCost = 0;
            bool closed = false;
        };

        struct OpenEntry
        {
            sf::Vector2i pos;
            int fCost;
            int gCost;

            bool operator>(const OpenEntry &other) const
            {
                if (fCost != other.fCost)
                    return fCost > other.fCost;
                return gCost < other.gCost;
            }
        };

        constexpr sf::Vector2i DIRS[8] = {
            { 1,  0}, {-1,  0}, { 0,  1}, { 0, -1},
            { 1,  1}, { 1, -1}, {-1,  1}, {-1, -1}};

        constexpr sf::Vector2i CARDINAL_DIRS[4] = {
            { 1,  0}, {-1,  0}, { 0,  1}, { 0, -1}};

        int Heuristic(sf::Vector2i a, sf::Vector2i b)
        {
            int dx = std::abs(a.x - b.x);
            int dy = std::abs(a.y - b.y);
            return 10 * (dx + dy) - 6 * std::min(dx, dy);
        }

        bool IsAdjacent(sf::Vector2i a, sf::Vector2i b)
        {
            return a != b && std::abs(a.x - b.x) <= 1 && std::abs(a.y - b.y) <= 1;
        }

        sf::Vector2i PosToStartTile(sf::Vector2f startPos)
        {
            return {
                static_cast<int>(std::floor(startPos.x + 0.5f)),
                static_cast<int>(std::floor(startPos.y + 0.5f))};
        }

        // Read-only tile type lookup that never generates chunks on the server.
        bool TryGetTileType(Planet *planet, sf::Vector2i tilePos, uint16_t &outType)
        {
            sf::Vector2i chunkPos = TileToChunkPos(tilePos);
            auto it = planet->chunks.find(chunkPos);
            if (it == planet->chunks.end() || !it->second)
                return false;

            sf::Vector2i local = tilePos - chunkPos * CHUNK_SIZE;
            outType = it->second->tiles[local.x][local.y].type;
            return true;
        }

        std::vector<sf::Vector2i> ReconstructPath(
            const std::unordered_map<sf::Vector2i, NodeRecord, ChunkHash> &records,
            sf::Vector2i startTile,
            sf::Vector2i endTile)
        {
            std::vector<sf::Vector2i> waypoints;
            sf::Vector2i trace = endTile;
            while (trace != startTile)
            {
                waypoints.push_back(trace);
                trace = records.at(trace).parent;
            }
            std::reverse(waypoints.begin(), waypoints.end());
            return waypoints;
        }
    }

    PathResult Pathfinder::FindPath(
        Planet *planet,
        sf::Vector2f startPos,
        sf::Vector2i goalTile,
        bool allowAdjacent,
        int maxExpandedNodes)
    {
        if (!planet)
            return {false, {}, goalTile};

        sf::Vector2i startTile = PosToStartTile(startPos);

        auto isGoal = [&](sf::Vector2i tile) -> bool
        {
            if (tile == goalTile)
                return true;
            return allowAdjacent && IsAdjacent(tile, goalTile);
        };

        if (isGoal(startTile))
            return {true, {}, goalTile};

        if (!allowAdjacent && !planet->IsTileWalkable(goalTile))
            return {false, {}, goalTile};

        std::priority_queue<OpenEntry, std::vector<OpenEntry>, std::greater<OpenEntry>> openSet;
        std::unordered_map<sf::Vector2i, NodeRecord, ChunkHash> records;
        records.reserve(256);

        records[startTile] = NodeRecord{startTile, 0, false};
        openSet.push(OpenEntry{startTile, Heuristic(startTile, goalTile), 0});

        int expanded = 0;

        while (!openSet.empty() && expanded < maxExpandedNodes)
        {
            OpenEntry current = openSet.top();
            openSet.pop();

            NodeRecord &currRec = records[current.pos];
            if (currRec.closed)
                continue;
            currRec.closed = true;
            expanded++;

            if (isGoal(current.pos))
            {
                return {true, ReconstructPath(records, startTile, current.pos), goalTile};
            }

            for (const sf::Vector2i &dir : DIRS)
            {
                sf::Vector2i next = current.pos + dir;
                bool isDiagonal = (dir.x != 0 && dir.y != 0);

                if (!planet->IsTileWalkable(next))
                    continue;

                if (isDiagonal)
                {
                    if (!planet->IsTileWalkable({current.pos.x + dir.x, current.pos.y}) ||
                        !planet->IsTileWalkable({current.pos.x, current.pos.y + dir.y}))
                    {
                        continue;
                    }
                }

                int stepCost = isDiagonal ? 14 : 10;
                int tentativeG = currRec.gCost + stepCost;

                auto it = records.find(next);
                if (it != records.end())
                {
                    if (it->second.closed || tentativeG >= it->second.gCost)
                        continue;
                    it->second.parent = current.pos;
                    it->second.gCost = tentativeG;
                }
                else
                {
                    records.emplace(next, NodeRecord{current.pos, tentativeG, false});
                }

                int fCost = tentativeG + Heuristic(next, goalTile);
                openSet.push(OpenEntry{next, fCost, tentativeG});
            }
        }

        return {false, {}, goalTile};
    }

    PathResult Pathfinder::FindPathToTileType(
        Planet *planet,
        sf::Vector2f startPos,
        uint16_t targetTileType,
        bool allowAdjacent,
        int maxExpandedNodes)
    {
        if (!planet)
            return {false, {}, {0, 0}};

        sf::Vector2i startTile = PosToStartTile(startPos);

        // Checks whether standing on `tile` satisfies the search, outputting the matched tile coordinate.
        auto matchesTarget = [&](sf::Vector2i tile, sf::Vector2i &matchedTile) -> bool
        {
            uint16_t type = 0;
            if (TryGetTileType(planet, tile, type) && type == targetTileType)
            {
                matchedTile = tile;
                return true;
            }
            if (allowAdjacent)
            {
                // Cardinal neighbors match ActionDestroyTile's 4 directions (up/right/down/left)
                for (const sf::Vector2i &dir : CARDINAL_DIRS)
                {
                    sf::Vector2i neighbor = tile + dir;
                    if (TryGetTileType(planet, neighbor, type) && type == targetTileType)
                    {
                        matchedTile = neighbor;
                        return true;
                    }
                }
            }
            return false;
        };

        sf::Vector2i matched{0, 0};
        if (matchesTarget(startTile, matched))
        {
            return {true, {}, matched};
        }

        // Uniform-cost outward search (Dijkstra, h = 0) since the target coordinate is unknown
        std::priority_queue<OpenEntry, std::vector<OpenEntry>, std::greater<OpenEntry>> openSet;
        std::unordered_map<sf::Vector2i, NodeRecord, ChunkHash> records;
        records.reserve(256);

        records[startTile] = NodeRecord{startTile, 0, false};
        openSet.push(OpenEntry{startTile, 0, 0});

        int expanded = 0;

        while (!openSet.empty() && expanded < maxExpandedNodes)
        {
            OpenEntry current = openSet.top();
            openSet.pop();

            NodeRecord &currRec = records[current.pos];
            if (currRec.closed)
                continue;
            currRec.closed = true;
            expanded++;

            if (matchesTarget(current.pos, matched))
            {
                return {true, ReconstructPath(records, startTile, current.pos), matched};
            }

            for (const sf::Vector2i &dir : DIRS)
            {
                sf::Vector2i next = current.pos + dir;
                bool isDiagonal = (dir.x != 0 && dir.y != 0);

                if (!planet->IsTileWalkable(next))
                    continue;

                if (isDiagonal)
                {
                    if (!planet->IsTileWalkable({current.pos.x + dir.x, current.pos.y}) ||
                        !planet->IsTileWalkable({current.pos.x, current.pos.y + dir.y}))
                    {
                        continue;
                    }
                }

                int stepCost = isDiagonal ? 14 : 10;
                int tentativeG = currRec.gCost + stepCost;

                auto it = records.find(next);
                if (it != records.end())
                {
                    if (it->second.closed || tentativeG >= it->second.gCost)
                        continue;
                    it->second.parent = current.pos;
                    it->second.gCost = tentativeG;
                }
                else
                {
                    records.emplace(next, NodeRecord{current.pos, tentativeG, false});
                }

                openSet.push(OpenEntry{next, tentativeG, tentativeG});
            }
        }

        return {false, {}, {0, 0}};
    }

    std::optional<sf::Vector2i> Pathfinder::FindTileOfType(
        Planet *planet,
        sf::Vector2f startPos,
        uint16_t targetTileType,
        int maxChunkRadius)
    {
        if (!planet || planet->chunks.empty())
            return std::nullopt;

        sf::Vector2i startTile = PosToStartTile(startPos);
        sf::Vector2i startChunk = TileToChunkPos(startTile);

        auto scanChunk = [&](sf::Vector2i chunkPos) -> std::optional<sf::Vector2i>
        {
            auto it = planet->chunks.find(chunkPos);
            if (it == planet->chunks.end() || !it->second)
                return std::nullopt;

            const Chunk &chunk = *it->second;
            for (int x = 0; x < CHUNK_SIZE; ++x)
            {
                for (int y = 0; y < CHUNK_SIZE; ++y)
                {
                    if (chunk.tiles[x][y].type == targetTileType)
                    {
                        return sf::Vector2i{
                            chunkPos.x * CHUNK_SIZE + x,
                            chunkPos.y * CHUNK_SIZE + y};
                    }
                }
            }
            return std::nullopt;
        };

        // Search in deterministic expanding chunk rings so lockstep peers pick the exact same tile
        for (int r = 0; r <= maxChunkRadius; ++r)
        {
            if (r == 0)
            {
                if (auto found = scanChunk(startChunk))
                    return found;
                continue;
            }

            for (int dx = -r; dx <= r; ++dx)
            {
                if (auto found = scanChunk({startChunk.x + dx, startChunk.y - r}))
                    return found;
                if (auto found = scanChunk({startChunk.x + dx, startChunk.y + r}))
                    return found;
            }
            for (int dy = -r + 1; dy <= r - 1; ++dy)
            {
                if (auto found = scanChunk({startChunk.x - r, startChunk.y + dy}))
                    return found;
                if (auto found = scanChunk({startChunk.x + r, startChunk.y + dy}))
                    return found;
            }
        }

        return std::nullopt;
    }
}