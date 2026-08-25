#include "Input.hpp"
#include "../Main.hpp"
#include "../imgui/imgui-SFML.h"
#include "../imgui/imgui.h"
Input::Input()
{
}
InputState Input::ProcessEvents(sf::RenderWindow &window)
{
	InputState result;
	result.keysDown = {};
	result.keysPressed = {};
	result.keysReleased = {};
	result.mouseButtonsDown = {};
	result.mouseButtonsPressed = {};
	result.mouseButtonsReleased = {};
	std::string typedText = "";
	result.scroll = {0.f, 0.f};
	bool overridden = false;
	while (const std::optional event = window.pollEvent())
	{
		// if ( !event->is<sf::Event::MouseMoved>() && !event->is<sf::Event::MouseMovedRaw>()){
		// ImGui::SFML::ProcessEvent(window, *event);
		// }
		if (!macro.active)
		{

			ImGui::SFML::ProcessEvent(window, *event);
		}
		if (event->is<sf::Event::Closed>())
		{
			window.close();
			exit(0);
		}

		if (const auto *keyPressed = event->getIf<sf::Event::KeyPressed>())
		{
			result.keysPressed.push_back(keyPressed->code);
			if (!currentState.Down(keyPressed->code))
			{
				currentState.keysDown.push_back(keyPressed->code);
			}
		}
		else if (const auto *keyReleased = event->getIf<sf::Event::KeyReleased>())
		{
			result.keysReleased.push_back(keyReleased->code);
			if (currentState.Down(keyReleased->code))
			{
				currentState.RemoveInput(keyReleased->code);
			}
		}
		else if (const auto *mb = event->getIf<sf::Event::MouseButtonPressed>())
		{
			if (!currentState.Down(mb->button))
			{
				currentState.mouseButtonsDown.push_back(mb->button);
				result.mouseButtonsPressed.push_back(mb->button);
			}
		}
		else if (const auto *mb = event->getIf<sf::Event::MouseButtonReleased>())
		{
			if (currentState.Down(mb->button))
			{
				currentState.RemoveInput(mb->button);
				result.mouseButtonsReleased.push_back(mb->button);
			}
		}
		else if (const auto *resized = event->getIf<sf::Event::Resized>())
		{
			result.windowResized = true;
		}
		else if (const auto *t = event->getIf<sf::Event::TextEntered>())
		{
			typedText += t->unicode;
		}
		else if (const auto *t = event->getIf<sf::Event::MouseWheelScrolled>())
		{
			result.scroll.y = t->delta;
		}
	}
	result.mouseIsBlocked = false;
	result.typedText = typedText;
	if (result.windowResized)
	{
		sf::Vector2u size = window.getSize();
		sf::View view(sf::FloatRect({0.f, 0.f}, {(float)size.x, (float)size.y}));
		window.setView(view);
	}

	result.mousePosition = (sf::Vector2f)(sf::Mouse::getPosition(window));
	result.mousePositionDelta = result.mousePosition - currentState.mousePosition;
	currentState.mousePosition = result.mousePosition;
	for (int i = 0; i < currentState.keysDown.size(); i++)
	{
		result.keysDown.push_back(currentState.keysDown[i]);
	}
	for (int i = 0; i < currentState.mouseButtonsDown.size(); i++)
	{
		result.mouseButtonsDown.push_back(currentState.mouseButtonsDown[i]);
	}
	if (ImGui::GetIO().WantCaptureKeyboard)
	{
		result.keysDown = {};
		result.keysPressed = {};
		result.keysReleased = {};
	}
	if (ImGui::GetIO().WantCaptureMouse)
	{
		result.mouseButtonsDown = {};
		result.mouseButtonsPressed = {};
		result.mouseButtonsReleased = {};
		result.scroll.y = 0;
	}
	return result;
}