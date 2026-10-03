#pragma once
#include "State.hpp"
#include "MacroRunner.hpp"
#include "CC/Server.hpp"

/// The program's window. The only one, hence a unique_ptr.
extern std::unique_ptr<sf::RenderWindow> window;

/// The screen currently running (menu, game, ...). Replace it to switch screens.
extern std::unique_ptr<Kosmic::State> state;

/// The game server, when this program is hosting a game. Null otherwise.
extern std::unique_ptr<cc::Server> server;

/// The macro (scripted input) that can drive the program. Inactive unless a script has been loaded.
extern Kosmic::Macro macro;