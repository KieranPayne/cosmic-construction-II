#pragma once
#include "../PCH.hpp"
#include "InputState.hpp"

class Input
{
private:
	InputState currentState;

public:
	Input();
	InputState ProcessEvents(sf::RenderWindow &window);
};