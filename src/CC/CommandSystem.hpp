#pragma once
#include "../PCH.hpp"
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace cc
{
    class Server;
    class Human;
    class Planet;
    struct Command;

    /**
     * @brief Everything a command needs while it runs: who sent it, its arguments, and helpers for
     *        replying and for finding things in the world. Created by CommandSystem for each command.
     */
    struct CommandContext
    {
        CommandContext(Server &server, uint64_t clientId) : server(server), clientId(clientId) {}

        /// The server the command runs on. Commands run on the server thread, so this is safe to use directly.
        Server &server;
        /// Id of the client who typed the command.
        uint64_t clientId;
        /// The words after the command name. Quoted text counts as one word.
        std::vector<std::string> args;
        /// The command being run. Set by CommandSystem.
        const Command *command = nullptr;

        // ---- replying (goes only to the sender, as a server message) ----

        void Reply(const std::string &message) const;
        void ReplyError(const std::string &message) const;
        /// Replies with the command's usage line.
        void ReplyUsage() const;

        // ---- reading arguments ----

        bool HasArg(size_t i) const { return i < args.size(); }
        /// @return false (and leaves `out` alone) if the argument is missing or not a whole number.
        bool ArgInt(size_t i, int &out) const;
        /// @return false (and leaves `out` alone) if the argument is missing or not a number.
        bool ArgFloat(size_t i, float &out) const;

        // ---- finding things in the world ----

        /// The planet the sending player is on, or null.
        Planet *PlayerPlanet() const;
        /// Index (in the planet's entity list) of the human nearest the sender's camera, or -1 if there is none.
        int NearestHumanIndex() const;
        /// The human at an entity index on the sender's planet, or null if there is none there or it isn't a human.
        Human *FindHuman(int entityIndex) const;
        /// Sends the current state of an entity to every joined client on the sender's planet.
        /// Call after a command changes an entity, so clients see the change.
        void SyncEntity(int entityIndex) const;
    };

    /// A chat command, for example "/learn". Add new ones with CommandSystem::Register().
    struct Command
    {
        /// What the player types after the slash. Not case sensitive. Must be one word.
        std::string name;
        /// Shown by /help and on a usage error, for example "/learn <tech> <amount> [human]".
        std::string usage;
        /// One line shown by /help.
        std::string description;
        /// If true, only the host can run it. Debugging commands should keep this on.
        bool hostOnly = true;
        /// The command's code. Report problems with ctx.ReplyError() and results with ctx.Reply().
        std::function<void(CommandContext &)> run;
    };

    /**
     * @brief Parses chat lines that start with "/" and runs the matching command.
     *
     * To add a command, write a function that fills in a Command and passes it to Register(). Group related
     * commands in their own Register function and call it from the server's constructor (see
     * RegisterBuiltinCommands()).
     */
    class CommandSystem
    {
    public:
        /// @brief Adds a command. A later command with the same name replaces the earlier one.
        void Register(Command command);

        /**
         * @brief Runs a chat line as a command if it starts with "/".
         * @return true if the line was a command (even a failed or unknown one), meaning it should not be
         *         shown as normal chat. false if it is ordinary chat.
         */
        bool TryExecute(Server &server, uint64_t clientId, const std::string &line);

        /// All registered commands, in registration order.
        const std::vector<Command> &Commands() const { return commands; }

        /// @brief Finds a command by name (any case), or null.
        const Command *Find(const std::string &name) const;

    private:
        std::vector<Command> commands;
        std::unordered_map<std::string, size_t> lookup;
    };

    /// @brief Splits a line into words on whitespace. Text in double quotes stays together as one word.
    std::vector<std::string> TokenizeCommand(const std::string &line);

    /// @brief Registers the built-in commands (help, learn). Add further groups the same way.
    void RegisterBuiltinCommands(CommandSystem &system);
}