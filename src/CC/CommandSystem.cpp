#include "CommandSystem.hpp"
#include "Server.hpp"
#include "Human.hpp"
#include "CSMessage.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>

namespace cc
{
    static std::string ToLower(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return s;
    }

    std::vector<std::string> TokenizeCommand(const std::string &line)
    {
        std::vector<std::string> tokens;
        std::string current;
        bool inQuotes = false;
        bool hasToken = false;

        for (char c : line)
        {
            if (c == '"')
            {
                inQuotes = !inQuotes;
                hasToken = true; // so "" gives an empty word
            }
            else if (std::isspace(static_cast<unsigned char>(c)) && !inQuotes)
            {
                if (hasToken)
                {
                    tokens.push_back(current);
                    current.clear();
                    hasToken = false;
                }
            }
            else
            {
                current += c;
                hasToken = true;
            }
        }
        if (hasToken)
        {
            tokens.push_back(current);
        }
        return tokens;
    }

    // ---------------- CommandContext ----------------

    void CommandContext::Reply(const std::string &message) const
    {
        sf::Packet p;
        p << (uint16_t)CSMessageType::SERVER_MESSAGE;
        p << message;
        server.SendToClient(clientId, p);
    }

    void CommandContext::ReplyError(const std::string &message) const
    {
        Reply("Error: " + message);
    }

    void CommandContext::ReplyUsage() const
    {
        if (command)
        {
            Reply("Usage: " + command->usage);
        }
    }

    bool CommandContext::ArgInt(size_t i, int &out) const
    {
        if (!HasArg(i))
        {
            return false;
        }
        const char *text = args[i].c_str();
        char *end = nullptr;
        long value = std::strtol(text, &end, 10);
        if (end == text || *end != '\0')
        {
            return false;
        }
        out = static_cast<int>(value);
        return true;
    }

    bool CommandContext::ArgFloat(size_t i, float &out) const
    {
        if (!HasArg(i))
        {
            return false;
        }
        const char *text = args[i].c_str();
        char *end = nullptr;
        float value = std::strtof(text, &end);
        if (end == text || *end != '\0')
        {
            return false;
        }
        out = value;
        return true;
    }

    Planet *CommandContext::PlayerPlanet() const
    {
        Server::ServerClient *client = server.GetClient(clientId);
        if (!client)
        {
            return nullptr;
        }
        int index = client->player.planet;
        if (index < 0 || index >= static_cast<int>(server.planets.size()))
        {
            return nullptr;
        }
        return server.planets[index].get();
    }

    int CommandContext::NearestHumanIndex() const
    {
        Planet *planet = PlayerPlanet();
        Server::ServerClient *client = server.GetClient(clientId);
        if (!planet || !client)
        {
            return -1;
        }

        sf::Vector2f camera = client->player.cameraPosition;
        int best = -1;
        float bestDistance = 0.f;
        for (int i = 0; i < static_cast<int>(planet->entities.size()); ++i)
        {
            Human *human = dynamic_cast<Human *>(planet->entities[i].get());
            if (!human)
            {
                continue;
            }
            // Entity positions are in tiles, the camera is in world pixels.
            float distance = (human->position * static_cast<float>(TILE_SIZE) - camera).lengthSquared();
            if (best == -1 || distance < bestDistance)
            {
                best = i;
                bestDistance = distance;
            }
        }
        return best;
    }

    Human *CommandContext::FindHuman(int entityIndex) const
    {
        Planet *planet = PlayerPlanet();
        if (!planet || entityIndex < 0 || entityIndex >= static_cast<int>(planet->entities.size()))
        {
            return nullptr;
        }
        return dynamic_cast<Human *>(planet->entities[entityIndex].get());
    }

    void CommandContext::SyncEntity(int entityIndex) const
    {
        Planet *planet = PlayerPlanet();
        Server::ServerClient *sender = server.GetClient(clientId);
        if (!planet || !sender || entityIndex < 0 || entityIndex >= static_cast<int>(planet->entities.size()))
        {
            return;
        }

        sf::Packet p;
        p << (uint16_t)CSMessageType::UPDATE_ENTITIES;
        p << (int)1;
        p << (int)entityIndex;
        AppendEntityToPacket(p, planet->entities[entityIndex].get());

        // Only clients that have joined and are on this planet can apply it.
        int planetIndex = sender->player.planet;
        for (auto &c : server.clients)
        {
            if (c.joined && c.player.planet == planetIndex)
            {
                server.SendToClient(c.id, p);
            }
        }
    }

    // ---------------- CommandSystem ----------------

    void CommandSystem::Register(Command command)
    {
        command.name = ToLower(command.name);
        auto it = lookup.find(command.name);
        if (it != lookup.end())
        {
            commands[it->second] = std::move(command);
            return;
        }
        lookup[command.name] = commands.size();
        commands.push_back(std::move(command));
    }

    const Command *CommandSystem::Find(const std::string &name) const
    {
        auto it = lookup.find(ToLower(name));
        return it == lookup.end() ? nullptr : &commands[it->second];
    }

    bool CommandSystem::TryExecute(Server &server, uint64_t clientId, const std::string &line)
    {
        if (line.empty() || line[0] != '/')
        {
            return false;
        }

        CommandContext ctx(server, clientId);
        std::vector<std::string> tokens = TokenizeCommand(line.substr(1));
        if (tokens.empty())
        {
            ctx.Reply("Type /help for a list of commands.");
            return true;
        }

        const Command *command = Find(tokens[0]);
        if (!command)
        {
            ctx.ReplyError("unknown command \"" + tokens[0] + "\". Type /help for a list.");
            return true;
        }

        if (command->hostOnly && !server.IsHost(clientId))
        {
            ctx.ReplyError("/" + command->name + " can only be used by the host.");
            return true;
        }

        ctx.command = command;
        ctx.args.assign(tokens.begin() + 1, tokens.end());
        command->run(ctx);
        return true;
    }
}