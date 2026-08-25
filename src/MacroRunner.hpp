#pragma once
#include "State.hpp"
namespace Kosmic
{
    enum MacroType
    {
        MACRO_DELAY_SECONDS,
        MACRO_DELAY_FRAMES,
    };
    class Macro;
    class MacroCommand
    {
    public:
        bool complete;
        virtual void Execute(State *state, Macro *macro);
        virtual void Reset();
        virtual void OnStart();
        MacroCommand();
    };
    class MacroPrint : public MacroCommand
    {
    public:
        std::string value;
        void Execute(State *state, Macro *macro);
    };
    class MacroDelay : public MacroCommand
    {
    public:
        MacroType type;
        double duration;
        double durationLeft;
        void Reset();
        void Execute(State *state, Macro *macro);
    };
    class MacroJump : public MacroCommand
    {
    public:
        std::string locationName;
        void Execute(State *state, Macro *macro);
    };
    class MacroRepeat : public MacroCommand
    {
    public:
        std::string locationName;
        int repCount;
        int repLeft;
        void Execute(State *state, Macro *macro);
        void OnStart();
    };
    class MacroEnd : public MacroCommand
    {
    public:
        void Execute(State *, Macro *macro);
    };
    class MacroKBPress : public MacroCommand
    {
    public:
        sf::Keyboard::Key index;
        void Execute(State *state, Macro *macro);
    };
    class MacroKBRelease : public MacroCommand
    {
    public:
        sf::Keyboard::Key index;
        void Execute(State *state, Macro *macro);
    };
    class MacroTypeText : public MacroCommand
    {
    public:
        std::string text;
        void Execute(State *state, Macro *macro);
    };
    class MacroMoveMouse : public MacroCommand
    {
    public:
        sf::Vector2f pos;
        void Execute(State *state, Macro *macro);
    };
    class MacroMBPress : public MacroCommand
    {
    public:
        sf::Mouse::Button index;
        void Execute(State *state, Macro *macro);
    };
    class MacroMBRelease : public MacroCommand
    {
    public:
        sf::Mouse::Button index;
        void Execute(State *state, Macro *macro);
    };
    class MacroWriteTimings : public MacroCommand
    {
    public:
        std::string path;
        void Execute(State *state, Macro *macro);
    };
    class MacroStartTimings : public MacroCommand
    {
    public:
        void Execute(State *state, Macro *macro);
    };
    class Macro
    {
    public:
        InputState inputState;
        std::unordered_map<std::string, int> jumpLocations;
        std::vector<int> locationStack;
        std::vector<MacroCommand *> commands;
        int index;
        bool active;
        void Parse(std::string str);
        void ParseFile(std::string path);
        void Execute(State *state);
        Macro();
        ~Macro();
    };
}