#include "Main.hpp"
#include "CC/MainMenu.hpp"
#include "CC/State.hpp"
#include "CC/TileInfo.hpp"
// #include "Civitron/ItemInfo.hpp"
#include "Input/Input.hpp"
#include "Timer.hpp"
#include "CC/EntityInfo.hpp"
#include "imgui/imgui-SFML.h"
#include "imgui/imgui.h"
#include <fstream>
#include <iostream>
// #include "Civitron/EntityInfo.hpp"
#include "MacroRunner.hpp"
// a unique pointer to the window object; this is unique to prevent accidentally creating multiple windows
std::unique_ptr<sf::RenderWindow> window;
// width and height of the window
int width = 1280;
int height = 720;
Kosmic::State *state = nullptr;

Kosmic::Macro macro;

// the main procedure that runs the program
int main()
{
	srand(time(NULL));
	window = std::make_unique<sf::RenderWindow>(sf::VideoMode({(unsigned int)width, (unsigned int)height}), "Cosmic Construction II");
	window->setFramerateLimit(9999);
	window->setVerticalSyncEnabled(false);
	// set the icon image that is displayed in the corner of the window
	sf::Image icon;
	if (!icon.loadFromFile("content/resources/images/icon.png"))
	{
		return 1;
	}

	window->setIcon({256, 256}, icon.getPixelsPtr());
	if (!ImGui::SFML::Init(*window))
		return -1;
	Input input;
	// state = new Civitron::State();
	state = new cc::MainMenu();
	state->renderTarget = window.get();
	sf::Clock deltaClock;
	cc::TileInfo::Init();
	cc::EntityInfo::Init();
	// Civitron::ItemInfo::Init();
	cc::EntityInfo::Build();
	// auto &io = ImGui::GetIO();
	// io.Fonts->Clear();
	// ImFont *font = io.Fonts->AddFontFromFileTTF("content/resources/fonts/default font.ttf", 20.f);
	// if (!ImGui::SFML::UpdateFontTexture())
	// {
		// window->close();
	// };
	macro.active = false;
	// macro.ParseFile("content/resources/macro2.txt");

	// tgui::Button::Ptr button = tgui::Button::create();
	// gui.add(button);
	while (window->isOpen())
	{
		InputState inputState = input.ProcessEvents(*window);
		auto time = deltaClock.restart();
		double dt = time.asSeconds();
		ImGui::SFML::Update(*window, time);
		inputState.DrawToWindow();
		if (macro.active)
		{
			macro.Execute(state);
			InputState copy = InputState(macro.inputState);
			inputState = copy;
		}
		state->Update(inputState, dt);
		window->clear(sf::Color(0, 0, 0));
		// window->clear(sf::Color(8, 38, 19));
		state->Render();
		ImGui::SFML::Render(*window);

		window->display();
	}
	return 0;
}
