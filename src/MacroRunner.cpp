#include "MacroRunner.hpp"
#include "CC/Utils.hpp"
#include "CC/SaveManager.hpp"
#include "State.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui-SFML.h"
#include "Main.hpp"
#include "Timer.hpp"
namespace Kosmic
{
    MacroCommand::MacroCommand()
    {
        complete = false;
    }
    void MacroCommand::OnStart()
    {
    }
    void MacroCommand::Execute(State *state, Macro *macro)
    {
    }
    void MacroCommand::Reset()
    {
        complete = false;
    }
    void MacroDelay::Execute(State *state, Macro *macro)
    {
        if (type == MACRO_DELAY_SECONDS)
        {
            durationLeft -= state->deltaTime;
            if (durationLeft < 0.0)
            {
                complete = true;
            }
        }
        else if (type == MACRO_DELAY_FRAMES)
        {
            durationLeft--;
            if (durationLeft < 0.0)
            {
                complete = true;
            }
        }
    }

    void MacroDelay::Reset()
    {
        MacroCommand::Reset();
        durationLeft = duration;
    }
    void MacroPrint::Execute(State *state, Macro *macro)
    {
        std::cout << value << std::endl;
        complete = true;
    }
    void MacroJump::Execute(State *state, Macro *macro)
    {
        macro->locationStack.push_back(macro->index + 1);
        macro->index = macro->jumpLocations[locationName] - 1;
        complete = true;
    }
    void MacroRepeat::Execute(State *state, Macro *macro)
    {
        if (repLeft > 0)
        {
            macro->locationStack.push_back(macro->index);
            macro->index = macro->jumpLocations[locationName] - 1;
            repLeft--;
        }
        else
        {
            repLeft = repCount;
        }
        complete = true;
    }
    void MacroEnd::Execute(State *state, Macro *macro)
    {
        if (macro->locationStack.size() > 0)
        {
            macro->index = macro->locationStack.back() - 1;
            macro->locationStack.pop_back();
        }
        else
        {
            macro->index = macro->commands.size() - 1;
        }
        complete = true;
    }
    void MacroRepeat::OnStart()
    {
        repLeft = repCount;
    }
    void MacroKBPress::Execute(State *state, Macro *macro)
    {
        macro->inputState.keysPressed.push_back(index);
        if (!macro->inputState.Down(index))
        {
            macro->inputState.keysDown.push_back(index);
        }
        ImGui::SFML::ProcessEvent(*window, sf::Event(sf::Event::KeyPressed{index}));
        complete = true;
    }
    void MacroKBRelease::Execute(State *state, Macro *macro)
    {
        macro->inputState.RemoveInput(index);
        macro->inputState.keysReleased.push_back(index);
        ImGui::SFML::ProcessEvent(*window, sf::Event(sf::Event::KeyReleased{index}));
        complete = true;
    }
    void MacroMBPress::Execute(State *state, Macro *macro)
    {
        macro->inputState.mouseButtonsPressed.push_back(index);
        if (!macro->inputState.Down(index))
        {
            macro->inputState.mouseButtonsDown.push_back(index);
        }
        ImGui::SFML::ProcessEvent(*window, sf::Event(sf::Event::MouseButtonPressed{index, {(int)macro->inputState.mousePosition.x, (int)macro->inputState.mousePosition.y}}));
        complete = true;
    }
    void MacroMBRelease::Execute(State *state, Macro *macro)
    {
        macro->inputState.RemoveInput(index);
        macro->inputState.mouseButtonsReleased.push_back(index);
        ImGui::SFML::ProcessEvent(*window, sf::Event(sf::Event::MouseButtonReleased{index, {(int)macro->inputState.mousePosition.x, (int)macro->inputState.mousePosition.y}}));
        complete = true;
    }
    void MacroMoveMouse::Execute(State *state, Macro *macro)
    {
        macro->inputState.mousePosition = pos;
        ImGui::SFML::ProcessEvent(*window, sf::Event(sf::Event::MouseMoved{{(int)pos.x, (int)pos.y}}));
        complete = true;
    }
    void MacroTypeText::Execute(State *state, Macro *macro)
    {
        macro->inputState.typedText = text;
        for (int i = 0; i < text.size(); i++)
        {
            sf::Event::TextEntered t{(char32_t)(text[i])};
            ImGui::SFML::ProcessEvent(*window, sf::Event(t));
        }
        complete = true;
    }
    void MacroWriteTimings::Execute(State *state, Macro *macro)
    {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm = *std::localtime(&t);

        std::ostringstream oss;
        oss << std::put_time(&tm, "%d-%m-%Y %H-%M-%S");
        std::string formatted = oss.str();
        std::string path = this->path + "\\" + formatted + ".txt";
        Timing::WriteToFile(path);
        complete = true;
    }
    void MacroStartTimings::Execute(State *state, Macro *macro)
    {
        Timing::timePerWrite = -0.01f;
        Timing::data = {};
        Timing::stack = {};
        Timing::currentIndent = 0;
        complete = true;
    }
    Macro::Macro()
    {
        commands = {};
    }
    void Macro::Parse(std::string str)
    {
        commands = {};
        auto lines = cc::Split(str, '\n');
        for (auto &line : lines)
        {
            // skip blank lines and comments
            if (line.size() == 0 || (line.size() >= 2 && line[0] == '/' && line[1] == '/'))
            {
                continue;
            }
            auto parts = cc::Split(line, ' ');
            if (parts[0] == "DELAY")
            {
                double duration = std::stod(parts[1]);
                MacroDelay *m = new MacroDelay();
                m->duration = duration;
                if (parts[2] == "S")
                {
                    m->type = MACRO_DELAY_SECONDS;
                }
                else if (parts[2] == "F")
                {
                    m->type = MACRO_DELAY_FRAMES;
                }
                commands.push_back(m);
            }
            else if (parts[0] == "PRINT")
            {
                std::string value = line.substr(std::string("PRINT ").size());
                MacroPrint *m = new MacroPrint();
                m->value = value;
                commands.push_back(m);
            }
            else if (line.back() == ':')
            {
                jumpLocations[line.substr(0, line.size() - 1)] = commands.size();
            }
            else if (parts[0] == "GOTO")
            {
                MacroJump *m = new MacroJump();
                m->locationName = line.substr(std::string("GOTO ").size());
                commands.push_back(m);
            }
            else if (parts[0] == "REPEAT")
            {
                MacroRepeat *m = new MacroRepeat();
                m->locationName = line.substr((parts[0] + parts[1]).size() + 2);
                m->repCount = std::stoi(parts[1]);
                commands.push_back(m);
            }
            else if (parts[0] == "END")
            {
                commands.push_back(new MacroEnd());
            }
            else if (parts[0] == "KBP")
            {
                MacroKBPress *m = new MacroKBPress();
                m->index = (sf::Keyboard::Key)(std::stoi(parts[1]));
                commands.push_back(m);
            }
            else if (parts[0] == "KBR")
            {
                MacroKBRelease *m = new MacroKBRelease();
                m->index = (sf::Keyboard::Key)(std::stoi(parts[1]));
                commands.push_back(m);
            }
            else if (parts[0] == "TYPE")
            {
                MacroTypeText *m = new MacroTypeText();
                m->text = line.substr(std::string("TYPE").size() + 1);
                commands.push_back(m);
            }
            else if (parts[0] == "MBP")
            {
                MacroMBPress *m = new MacroMBPress();
                m->index = (sf::Mouse::Button)(std::stoi(parts[1]));
                commands.push_back(m);
            }
            else if (parts[0] == "MBR")
            {
                MacroMBRelease *m = new MacroMBRelease();
                m->index = (sf::Mouse::Button)(std::stoi(parts[1]));
                commands.push_back(m);
            }
            else if (parts[0] == "MOVE")
            {
                MacroMoveMouse *m = new MacroMoveMouse();
                m->pos = {std::stof(parts[1]), std::stof(parts[2])};
                commands.push_back(m);
            }
            else if (parts[0] == "TIMING")
            {
                if (parts[1] == "START")
                {
                    MacroStartTimings *m = new MacroStartTimings();
                    commands.push_back(m);
                }
                else if (parts[1] == "END")
                {
                    std::string path = line.substr(std::string("TIMING END ").size());
                    MacroWriteTimings *m = new MacroWriteTimings();
                    m->path = path;
                    commands.push_back(m);
                }
            }
        }
        index = 0;
        for (MacroCommand *m : commands)
        {
            m->Reset();
            m->OnStart();
        }
        active = true;
        ImGuiIO &io = ImGui::GetIO();
        io.AddFocusEvent(true);
    }
    void Macro::ParseFile(std::string path)
    {
        std::string data = cc::SaveManager::ReadData(path);
        Parse(data);
    }
    void Macro::Execute(State *state)
    {
        if (!active)
        {
            return;
        }
        InputState newState;
        newState.keysDown = std::vector<sf::Keyboard::Key>(inputState.keysDown);
        newState.mouseButtonsDown = std::vector<sf::Mouse::Button>(inputState.mouseButtonsDown);
        newState.mousePosition = inputState.mousePosition;
        inputState = newState;

        if (index < commands.size())
        {
            MacroCommand *m = commands[index];
            m->Execute(state, this);
            if (m->complete)
            {
                m->Reset();
                index++;
                if (index == commands.size())
                {
                    std::cout << "macro complete";
                    active = false;
                }
            }
        }
    }
    Macro::~Macro()
    {
        for (int i = 0; i < commands.size(); i++)
        {
            delete commands[i];
        }
    }
}