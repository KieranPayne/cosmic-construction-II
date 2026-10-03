#pragma once
#include "State.hpp"
#include "MacroRunner.hpp"
#include "CC/Server.hpp"
extern std::unique_ptr<sf::RenderWindow> window;
extern std::unique_ptr<Kosmic::State> state;
extern std::unique_ptr<cc::Server> server;
extern Kosmic::Macro macro;