#include "World.hpp"
#include "CSMessage.hpp"
#include "SaveManager.hpp"
#include "Utils.hpp"
#include "Client.hpp"
namespace cc
{
    World::World()
    {
        planets.push_back(std::make_unique<Planet>());
        RegisterBuiltinCommands(commandSystem);
    }
    void World::Tick()
    {
        for (auto &p : planets)
        {
            p->Tick();
        }
        currentTick++;
    }
    void World::EnactInputBundle(InputBundle &bundle)
    {
        std::string username = bundle.username;
        // PlayerData* p = nullptr;
        // playerdata
        if (bundle.playerData.cameraZoom != 0)
        {
            for (int i = 0; i < players.size(); i++)
            {
                if (players[i].username == username)
                {
                    players[i] = bundle.playerData;
                    break;
                }
            }
        }
        // tiles set
        Planet *planet = planets[bundle.playerData.planet].get();
        for (int i = 0; i < bundle.tilesChanged.size(); i++)
        {
            planet->SetTileAt(bundle.tilesChanged[i]->position, bundle.tilesChanged[i]->tile, bundle.tilesChanged[i]->tileEntity.release());
        }
        // entities added
        for (int i = 0; i < bundle.entitiesAdded.size(); i++)
        {
            planet->AddEntity(bundle.entitiesAdded[i].release());
        }
        // entities changed
        for (int i = 0; i < bundle.entitiesChanged.size(); i++)
        {
            planet->ChangeEntity(bundle.changeIndices[i], bundle.entitiesChanged[i].release());
        }

        // chat messages
        for (auto &msg : bundle.chatMessages)
        {
            if (!commandSystem.TryExecute(this, &bundle.playerData, msg.message))
            {
                if (client != nullptr)
                {
                    client->LogMessage(msg.message, msg.origin, msg.playerUsername);
                }
            }
        }
        if (bundle.knowledgeDatabaseChanged)
        {
            knowledgeDatabase = std::move(*bundle.knowledgeDatabaseChanged);
            knowledgeDatabase.BuildOutgoingAndIncoming();
            bundle.knowledgeDatabaseChanged.reset();

            if (server != nullptr)
            {
                if (SaveManager::debuggingKnowledgeGraph)
                    SaveKnowledgeDatabase(SaveManager::GetSavedataDir() + "/knowledge graph");
                else
                    SaveKnowledgeDatabase(SaveManager::savePath + "/knowledge graph");
            }
            if (client != nullptr)
            {
                client->OnKnowledgeDatabaseUpdated();
            }
        }
        GenerateChunksInView(&bundle.playerData);
    }

    void World::GenerateStateCheck(sf::Packet &packet)
    {
        packet << currentTick;

        Serializer ks(Serializer::Mode::WRITE, Serializer::Format::BINARY);
        knowledgeDatabase.Serialize(ks);
        auto kbBinary = ks.binary();
        packet << ComputeChecksum(kbBinary.data(), kbBinary.size());

        packet << static_cast<uint64_t>(planets.size());
        for (auto &planet : planets)
        {
            planet->GenerateStateCheck(packet);
        }
    }
    void World::AppendJoinData(sf::Packet &packet)
    {
        packet << (uint64_t)planets.size();
        for (auto &p : planets)
        {
            packet << (uint64_t)p->chunks.size();
            for (auto &c : p->chunks)
            {
                packet << c.second->position.x << c.second->position.y;
                auto b = c.second->GetByteData();
                packet << (uint64_t)b.size();
                packet.append(b.data(), b.size());
            }
            // send entities
            uint64_t numEntities = (uint64_t)p->entities.size();
            packet << numEntities;
            for (int i = 0; i < numEntities; i++)
            {
                AppendEntityToPacket(packet, p->entities[i].get());
            }
            p->generator.AppendToPacket(packet);
        }
        Serializer s(Serializer::Mode::WRITE, Serializer::Format::BINARY);
        int n = players.size();
        s.field("n", n);
        for (int i = 0; i < n; i++)
            s.field(std::to_string(i), players[i]);
        auto data = s.binary();
        // put number of bytes in packet
        packet << (uint64_t)data.size();
        // put bytes into packet
        packet.append(data.data(), data.size());
        packet << currentTick;
        packet << seed;
        Serializer db(Serializer::Mode::WRITE, Serializer::Format::BINARY);
        knowledgeDatabase.Serialize(db);
        AppendSerializerToPacket(packet, db);
    }
    void World::LoadJoinData(sf::Packet &packet)
    {
        uint64_t numPlanets;
        packet >> numPlanets;
        while (numPlanets > planets.size())
        {
            planets.push_back(std::make_unique<Planet>());
        }
        for (int i = 0; i < numPlanets; i++)
        {
            uint64_t numChunks;
            packet >> numChunks;
            for (int j = 0; j < numChunks; j++)
            {
                sf::Vector2i pos;
                packet >> pos.x >> pos.y;
                auto b = ReadBytesFromPacket(packet);
                Chunk *c = new Chunk(pos);
                c->LoadByteData(b);
                planets[i]->chunks[pos] = std::unique_ptr<Chunk>(c);
            }
            uint64_t numEntities;
            packet >> numEntities;
            for (int j = 0; j < numEntities; j++)
            {
                Entity *e = LoadEntityFromPacket(packet);
                planets[i]->AddEntity(e);
            }
            planets[i]->generator.LoadFromPacket(packet);
        }
        // players currently on the server, including this one
        auto playerData = ReadBytesFromPacket(packet);
        Serializer s(Serializer::Mode::READ, Serializer::Format::BINARY, {}, playerData);

        int numPlayers;
        s.field("n", numPlayers);
        for (int i = 0; i < numPlayers; i++)
        {
            PlayerData p;
            s.field(std::to_string(i), p);
            // if (p.username == SaveManager::username)
            // {
            // 	// this is us: restore our saved planet and camera
            // 	activePlanet = p.planet;
            // 	planets[activePlanet]->camera.position = p.cameraPosition;
            // 	planets[activePlanet]->camera.targetZoom = p.cameraZoom;
            // 	continue;
            // }
            players.push_back(p);
            // AddOtherPlayer(p);
        }
        packet >> currentTick;
        packet >> seed;
        SetSeed(seed);
        Serializer db = ReadSerializerFromPacket(packet);
        knowledgeDatabase.Serialize(db);
    }
    void World::Save()
    {
        for (auto &p : planets)
        {
            p->Save();
        }
        for (int i = 0; i < players.size(); i++)
        {
            SavePlayer(i);
        }
        Serializer s(Serializer::Mode::WRITE, SaveManager::saveFormat);
        Serialize(s);
        SaveManager::WriteSerializerToFile(s, SaveManager::savePath + "/world");
    }
    void World::Load()
    {
        Serializer s = SaveManager::LoadSerializerFromFile(SaveManager::savePath + "/world");
        Serialize(s);
        for (auto &p : planets)
        {
            p->Load();
        }
    }
    void World::Serialize(Serializer &s)
    {
        s.field("currentTick", currentTick);
        s.field("playerHistory", playerHistory);
        int planetCount = planets.size();
        s.field("planetCount", planetCount);
        while (planets.size() < planetCount)
        {
            planets.push_back(std::make_unique<Planet>());
        } 
        s.field("seed", seed);
        SetSeed(seed);
    }
    void World::GenerateChunksInView(PlayerData *player)
    {
        Planet *planet = planets[player->planet].get();
        constexpr float chunkSizePixels = static_cast<float>(CHUNK_SIZE * TILE_SIZE);

        // Visible rectangle in world pixels
        sf::Vector2f viewSize = player->resolution * player->cameraZoom;
        sf::Vector2f topLeftWorld = player->cameraPosition - viewSize / 2.f;
        sf::Vector2f bottomRightWorld = topLeftWorld + viewSize;
        int paddingChunks = 10;

        // Convert world-pixel bounds to chunk coordinates (floor works for negative coords too)
        int minX = static_cast<int>(std::floor(topLeftWorld.x / chunkSizePixels)) - paddingChunks;
        int minY = static_cast<int>(std::floor(topLeftWorld.y / chunkSizePixels)) - paddingChunks;
        int maxX = static_cast<int>(std::floor(bottomRightWorld.x / chunkSizePixels)) + paddingChunks;
        int maxY = static_cast<int>(std::floor(bottomRightWorld.y / chunkSizePixels)) + paddingChunks;

        std::vector<sf::Vector2i> chunks;
        chunks.reserve(static_cast<size_t>((maxX - minX + 1) * (maxY - minY + 1)));

        for (int x = minX; x <= maxX; ++x)
        {
            for (int y = minY; y <= maxY; ++y)
            {
                chunks.push_back({x, y});
            }
        }
        for (auto &c : chunks)
        {
            if (!planet->chunks.contains(c))
            {
                planet->GenerateChunk(c);
            }
        }
    }
    void World::SavePlayer(int index)
    {
        for (int i = 0; i < playerHistory.size(); i++)
        {
            if (playerHistory[i].username == players[index].username)
            {
                playerHistory[i] = players[index];
                return;
            }
        }
        playerHistory.push_back(players[index]);
    }
    void World::SetSeed(uint64_t seed)
    {
        this->seed = seed;
        for (int i = 0; i < planets.size(); i++)
        {
            planets[i]->SetSeed(seed + i);
        }
    }
    void World::AddPlayer(PlayerData player)
    {
        players.push_back(player);
    }
    void World::ShowMessageToClient(std::string senderName, std::string message)
    {
        if (client != nullptr)
        {
            client->LogMessage(message, ChatMessageOrigin::PLAYER, senderName);
        }
    }
    void World::LoadKnowledgeDatabase(std::string path)
    {
        Serializer s = SaveManager::LoadSerializerFromFile(path);
        knowledgeDatabase.Serialize(s);
    }
    void World::SaveKnowledgeDatabase(std::string path)
    {
        Serializer s(Serializer::Mode::WRITE, SaveManager::saveFormat);
        knowledgeDatabase.Serialize(s);
        SaveManager::WriteSerializerToFile(s, path);
    }
}