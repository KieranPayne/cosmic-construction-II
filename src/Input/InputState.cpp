#include "InputState.hpp"
#include "../json.hpp"
#include "../imgui/imgui.h"
#include "../Main.hpp"
InputState::InputState()
{
	keysPressed = {};
	keysDown = {};
	keysReleased = {};
	mouseButtonsPressed = {};
	mouseButtonsDown = {};
	mouseButtonsReleased = {};
}

bool InputState::Down(sf::Keyboard::Key key)
{
	return getIndex(keysDown, key) != -1;
}
bool InputState::Pressed(sf::Keyboard::Key key)
{
	return getIndex(keysPressed, key) != -1;
}
bool InputState::Released(sf::Keyboard::Key key)
{
	return getIndex(keysReleased, key) != -1;
}
bool InputState::Down(sf::Mouse::Button button)
{
	return getIndex(mouseButtonsDown, button) != -1;
}
bool InputState::Pressed(sf::Mouse::Button button)
{
	return getIndex(mouseButtonsPressed, button) != -1;
}
bool InputState::Released(sf::Mouse::Button button)
{
	return getIndex(mouseButtonsReleased, button) != -1;
}
void InputState::RemoveInput(sf::Keyboard::Key key)
{
	int index = getIndex(keysPressed, key);
	if (index != -1)
	{
		keysPressed.erase(keysPressed.begin() + index);
	}
	index = getIndex(keysDown, key);
	if (index != -1)
	{
		keysDown.erase(keysDown.begin() + index);
	}
	index = getIndex(keysReleased, key);
	if (index != -1)
	{
		keysReleased.erase(keysReleased.begin() + index);
	}
}
void InputState::RemoveInput(sf::Mouse::Button button)
{
	int index = getIndex(mouseButtonsPressed, button);
	if (index != -1)
	{
		mouseButtonsPressed.erase(mouseButtonsPressed.begin() + index);
	}
	index = getIndex(mouseButtonsDown, button);
	if (index != -1)
	{
		mouseButtonsDown.erase(mouseButtonsDown.begin() + index);
	}
	index = getIndex(mouseButtonsReleased, button);
	if (index != -1)
	{
		mouseButtonsReleased.erase(mouseButtonsReleased.begin() + index);
	}
}

template <typename T>
int InputState::getIndex(std::vector<T> &arr, T &item)
{
	if (arr.size() == 0)
	{
		return -1;
	}
	for (int i = 0; i < arr.size(); i++)
	{
		if (arr[i] == item)
		{
			return i;
		}
	}
	return -1;
}

void InputState::DrawToWindow()
{
	if (macro.active)
	{
		return;
	}
	ImGui::Begin("input");
	nlohmann::json j;
	j["mouse pos"] = {mousePosition.x, mousePosition.y};
	j["keys down"] = keysDown;
	j["mouse buttons down"] = mouseButtonsDown;
	ImGui::Text(j.dump(1).c_str());
	ImGui::End();
}