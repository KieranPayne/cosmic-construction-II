#pragma once
#include "../PCH.hpp"

class InputState
{
public:
	std::vector<sf::Keyboard::Key> keysPressed;
	std::vector<sf::Keyboard::Key> keysDown;
	std::vector<sf::Keyboard::Key> keysReleased;
	std::vector<sf::Mouse::Button> mouseButtonsPressed;
	std::vector<sf::Mouse::Button> mouseButtonsDown;
	std::vector<sf::Mouse::Button> mouseButtonsReleased;
	sf::Vector2f mousePosition;
	sf::Vector2f mousePositionDelta;
	std::string typedText;
	sf::Vector2f scroll;
	bool windowResized = false;
	bool mouseIsBlocked;
	InputState();
	bool Down(sf::Keyboard::Key key);
	bool Pressed(sf::Keyboard::Key key);
	bool Released(sf::Keyboard::Key key);
	bool Down(sf::Mouse::Button button);
	bool Pressed(sf::Mouse::Button button);
	bool Released(sf::Mouse::Button button);
	void RemoveInput(sf::Keyboard::Key key);
	void RemoveInput(sf::Mouse::Button button);
	template <typename T>
	int getIndex(std::vector<T> &arr, T &item);
	void DrawToWindow();
};