#pragma once
#include "State.hpp"
namespace Kosmic
{
    /**
     * @brief The unit a MacroDelay counts in.
     */
    enum MacroType
    {
        /// Wait for a number of seconds.
        MACRO_DELAY_SECONDS,
        /// Wait for a number of frames.
        MACRO_DELAY_FRAMES,
    };

    class Macro;

    /**
     * @brief One step of a macro. The macro runs the current command every frame until it is complete.
     *
     * Derived classes do the actual work in Execute().
     */
    class MacroCommand
    {
    public:
        /// Set to true by Execute() when the command is finished and the macro can move on.
        bool complete;

        /**
         * @brief Does the command's work for this frame. Does nothing in the base class.
         * @param state The current state (gives access to the frame time).
         * @param macro The macro running the command.
         */
        virtual void Execute(State *state, Macro *macro);

        /// @brief Gets the command ready to run again by clearing `complete`.
        virtual void Reset();

        /// @brief Called once when the macro is loaded, before it first runs. Does nothing in the base class.
        virtual void OnStart();

        /// @brief Creates a command that is not complete.
        MacroCommand();
    };

    /// Prints text to the console.
    class MacroPrint : public MacroCommand
    {
    public:
        /// The text to print.
        std::string value;
        void Execute(State *state, Macro *macro);
    };

    /// Waits for a length of time or number of frames.
    class MacroDelay : public MacroCommand
    {
    public:
        /// Whether `duration` is in seconds or frames.
        MacroType type;

        /// How long to wait.
        double duration;

        /// How much of the wait is left. Reset to `duration` by Reset().
        double durationLeft;

        void Reset();
        void Execute(State *state, Macro *macro);
    };

    /// Jumps to a label. The macro remembers where it came from, so END can return.
    class MacroJump : public MacroCommand
    {
    public:
        /// The name of the label to jump to.
        std::string locationName;
        void Execute(State *state, Macro *macro);
    };

    /// Runs the code at a label a set number of times, then carries on after this command.
    class MacroRepeat : public MacroCommand
    {
    public:
        /// The name of the label whose code is repeated. That code should finish with END.
        std::string locationName;

        /// How many times to repeat.
        int repCount;

        /// How many repeats are still to do.
        int repLeft;

        void Execute(State *state, Macro *macro);

        /// @brief Sets `repLeft` to `repCount`.
        void OnStart();
    };

    /// Returns to where the last jump or repeat came from, or finishes the macro if there is nowhere to return to.
    class MacroEnd : public MacroCommand
    {
    public:
        void Execute(State *, Macro *macro);
    };

    /// Presses a keyboard key (and holds it until a MacroKBRelease).
    class MacroKBPress : public MacroCommand
    {
    public:
        /// The key to press.
        sf::Keyboard::Key index;
        void Execute(State *state, Macro *macro);
    };

    /// Releases a keyboard key.
    class MacroKBRelease : public MacroCommand
    {
    public:
        /// The key to release.
        sf::Keyboard::Key index;
        void Execute(State *state, Macro *macro);
    };

    /// Types a piece of text, as if entered on the keyboard.
    class MacroTypeText : public MacroCommand
    {
    public:
        /// The text to type.
        std::string text;
        void Execute(State *state, Macro *macro);
    };

    /// Moves the mouse.
    class MacroMoveMouse : public MacroCommand
    {
    public:
        /// Where to move the mouse to, in window pixels.
        sf::Vector2f pos;
        void Execute(State *state, Macro *macro);
    };

    /// Presses a mouse button (and holds it until a MacroMBRelease).
    class MacroMBPress : public MacroCommand
    {
    public:
        /// The button to press.
        sf::Mouse::Button index;
        void Execute(State *state, Macro *macro);
    };

    /// Releases a mouse button.
    class MacroMBRelease : public MacroCommand
    {
    public:
        /// The button to release.
        sf::Mouse::Button index;
        void Execute(State *state, Macro *macro);
    };

    /// Ends timing and saves the results to a file named after the current date and time.
    class MacroWriteTimings : public MacroCommand
    {
    public:
        /// The folder to save into.
        std::string path;
        void Execute(State *state, Macro *macro);
    };

    /// Starts a fresh timing run, with automatic writing turned off.
    class MacroStartTimings : public MacroCommand
    {
    public:
        void Execute(State *state, Macro *macro);
    };

    /**
     * @brief A script of commands that feeds fake keyboard and mouse input to the program, for repeatable testing.
     *
     * Scripts are text, one command per line. Blank lines and lines starting with "//" are ignored.
     *  - `DELAY <amount> S|F` waits that many seconds (S) or frames (F)
     *  - `PRINT <text>` prints text
     *  - `<name>:` defines a label
     *  - `GOTO <label>` jumps to a label
     *  - `REPEAT <count> <label>` runs the code at a label that many times
     *  - `END` returns from a GOTO or REPEAT, or finishes the macro
     *  - `KBP <key>` / `KBR <key>` presses / releases a key, given as an SFML key number
     *  - `MBP <button>` / `MBR <button>` presses / releases a mouse button, given as an SFML button number
     *  - `MOVE <x> <y>` moves the mouse
     *  - `TYPE <text>` types text
     *  - `TIMING START` and `TIMING END <folder>` start timing and save the results
     */
    class Macro
    {
    public:
        /// The input the macro is producing this frame. Used in place of real input while the macro is active.
        InputState inputState;

        /// The command number each label points to, by label name.
        std::unordered_map<std::string, int> jumpLocations;

        /// Command numbers to return to, most recent last. Pushed by GOTO and REPEAT, popped by END.
        std::vector<int> locationStack;

        /// The commands, in order. The macro owns them.
        std::vector<MacroCommand *> commands;

        /// The number of the command currently running.
        int index;

        /// Whether the macro is running. Becomes false when the last command finishes.
        bool active;

        /**
         * @brief Reads a script and starts the macro from the first command.
         * @param str The script text.
         */
        void Parse(std::string str);

        /**
         * @brief Reads a script from a file and starts the macro.
         * @param path The script file.
         */
        void ParseFile(std::string path);

        /**
         * @brief Runs the current command, and moves to the next when it is complete. Does nothing if not active.
         *
         * Keys and buttons that were being held stay held; presses and releases only last one frame.
         * @param state The current state.
         */
        void Execute(State *state);

        Macro();
        ~Macro();
    };
}