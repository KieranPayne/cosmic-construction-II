#include "CommandSystem.hpp"
#include "Server.hpp"
#include "Human.hpp"
#include "CSMessage.hpp"
#include <cctype>
#include <cstdio>
#include <cstdlib>

namespace cc
{
    static std::string Lower(std::string s)
    {
        for (auto &c : s)
        {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return s;
    }

    static std::string Fixed(float value)
    {
        char buffer[32];
        std::snprintf(buffer, sizeof(buffer), "%.3f", value);
        return buffer;
    }

    /// A technology given as a node index ("7") or its exact name, in any case ("stone knife"). -1 if not found.
    static int FindTech(const KnowledgeDatabase &database, const std::string &text)
    {
        char *end = nullptr;
        long value = std::strtol(text.c_str(), &end, 10);
        if (end != text.c_str() && *end == '\0')
        {
            return (value >= 0 && value < database.NodeCount()) ? static_cast<int>(value) : -1;
        }

        std::string wanted = Lower(text);
        for (int i = 0; i < database.NodeCount(); ++i)
        {
            if (Lower(database.GetNode(i).GetName()) == wanted)
            {
                return i;
            }
        }
        return -1;
    }

    static void RunHelp(CommandContext &ctx)
    {
        CommandSystem &system = ctx.world->commandSystem;

        if (ctx.HasArg(0))
        {
            const Command *command = system.Find(ctx.args[0]);
            if (!command)
            {
                ctx.ReplyError("unknown command \"" + ctx.args[0] + "\".");
                return;
            }
            ctx.Reply(command->usage + "\n" + command->description);
            return;
        }

        std::string message = "Commands:";
        for (const Command &command : system.Commands())
        {
            message += "\n" + command.usage + " - " + command.description;
        }
        ctx.Reply(message);
    }

    static void RunLearn(CommandContext &ctx)
    {
        //TODO: fix this
        KnowledgeDatabase &database = ctx.world->knowledgeDatabase;

        float amount;
        if (!ctx.HasArg(0) || !ctx.ArgFloat(1, amount))
        {
            ctx.ReplyUsage();
            return;
        }

        int techId = FindTech(database, ctx.args[0]);
        if (techId == -1)
        {
            ctx.ReplyError("no technology called \"" + ctx.args[0] + "\".");
            return;
        }

        // The target is the human named by entity index, or the one nearest the sender's camera.
        int entityIndex;
        if (ctx.HasArg(2))
        {
            if (!ctx.ArgInt(2, entityIndex))
            {
                ctx.ReplyUsage();
                return;
            }
        }
        else
        {
            entityIndex = ctx.NearestHumanIndex();
        }

        Human *human = ctx.FindHuman(entityIndex);
        if (!human)
        {
            ctx.ReplyError(ctx.HasArg(2) ? "entity " + std::to_string(entityIndex) + " is not a human."
                                         : "there is no human on this planet.");
            return;
        }

        std::vector<HumanKnowledge> before = human->knowledge;
        ctx.world->knowledgeSystem.GainUnderstanding(*human, techId, amount);

        // Report everything that changed, including what spread to related technologies.
        std::string message = "Human " + std::to_string(entityIndex) + " gained understanding:";
        bool anyChange = false;
        for (const HumanKnowledge &now : human->knowledge)
        {
            float old = 0.f;
            for (const HumanKnowledge &k : before)
            {
                if (k.techId == now.techId)
                {
                    old = k.understanding;
                    break;
                }
            }
            if (now.understanding - old > 0.000001f)
            {
                std::string name = database.GetNode(now.techId).GetName();
                message += "\n  " + (name.empty() ? "(unnamed)" : name) + " [" + std::to_string(now.techId) +
                           "]: " + Fixed(old) + " -> " + Fixed(now.understanding);
                anyChange = true;
            }
        }
        if (!anyChange)
        {
            message += "\n  nothing changed (already at the maximum?)";
        }

        ctx.Reply(message);
        // ctx.SyncEntity(entityIndex);
    }

    void RunDestroyTile(CommandContext &ctx)
    {
        int x, y;
        if (!ctx.ArgInt(0, x) || !ctx.ArgInt(1, y))
        {
            ctx.ReplyUsage();
            return;
        }

        Planet *planet = ctx.PlayerPlanet();
        if (!planet)
        {
            ctx.ReplyError("you are not on a planet.");
            return;
        }

        sf::Vector2i tilePos(x, y);
        planet->DestroyTile(tilePos);
        // sf::Vector2i chunkPos = TileToChunkPos(tilePos);
        // sf::Vector2i subChunkPos = tilePos - chunkPos * CHUNK_SIZE;

        // // The server generates chunks on demand, like GetTileAt does.
        // if (!planet->chunks.contains(chunkPos))
        // {
        //     planet->GenerateChunk(chunkPos);
        // }
        // Chunk *chunk = planet->chunks[chunkPos].get();

        // chunk->DestroyTile(planet,subChunkPos); // adjust to DestroyTile's real signature

        // Tell the clients on this planet what the tile is now. Without this only the server sees the change.
        // uint16_t newType = (uint16_t)chunk->tiles[subChunkPos.x][subChunkPos.y].type;
        // int planetIndex = ctx.playerData->planet;
        
        // sf::Packet p;
        // p << (uint16_t)CSMessageType::SET_TILES;
        // p << (int)1;
        // p << x << y << newType << false; // false: no tile entity
        // for (auto &c : ctx.server.clients)
        // {
        //     if (c.joined && c.player.planet == planetIndex)
        //     {
        //         c.outgoing.push_back(p);
        //     }
        // }

        ctx.Reply("Destroyed tile (" + std::to_string(x) + ", " + std::to_string(y) + ").");
    }

    void RegisterBuiltinCommands(CommandSystem &system)
    {
        Command help;
        help.name = "help";
        help.usage = "/help [command]";
        help.description = "Lists commands, or describes one.";
        help.hostOnly = false;
        help.run = RunHelp;
        system.Register(help);

        Command learn;
        learn.name = "learn";
        learn.usage = "/learn <tech> <amount> [human]";
        learn.description = "Gives a human understanding of a technology (by index or name, in quotes if it has spaces). "
                            "Related technologies gain a share. The human defaults to the one nearest your camera; "
                            "otherwise give its entity index.";
        learn.run = RunLearn;
        system.Register(learn);

        Command destroyTile;
        destroyTile.name = "destroytile";
        destroyTile.usage = "/destroytile <x> <y>";
        destroyTile.description = "calls the on destroy function for a tile at the coordinates and given and erases it.";
        destroyTile.run = RunDestroyTile;
        system.Register(destroyTile);
    }
}