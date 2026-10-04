#include "Main.hpp"
#include "CC/MainMenu.hpp"
#include "CC/UsernameMenu.hpp"
#include "CC/TileInfo.hpp"
#include "Input/Input.hpp"
#include "Timer.hpp"
#include "CC/EntityInfo.hpp"
#include "imgui/imgui-SFML.h"
#include "imgui/imgui.h"
#include <fstream>
#include <iostream>
#include "MacroRunner.hpp"
// a unique pointer to the window object; this is unique to prevent accidentally creating multiple windows
std::unique_ptr<sf::RenderWindow> window;
// width and height of the window
int width = 1280;
int height = 720;
std::unique_ptr<Kosmic::State> state;
std::unique_ptr<cc::Server> server;
Kosmic::Macro macro;

// the main procedure that runs the program
int main()
{
	srand(time(NULL));
	sf::ContextSettings settings;
	settings.antiAliasingLevel = 2;
	window = std::make_unique<sf::RenderWindow>(
		sf::VideoMode({(unsigned int)width, (unsigned int)height}),
		"Cosmic Construction II",
		sf::Style::Default,
		sf::State::Windowed,
		settings);
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
	cc::UsernameMenu *usernameMenu = new cc::UsernameMenu(window.get());
	if (usernameMenu->CheckForExisting())
	{
		delete usernameMenu;
		state = std::unique_ptr<Kosmic::State>(new cc::MainMenu(window.get()));
	}
	else
	{
		state = std::unique_ptr<Kosmic::State>(usernameMenu);
	}
	sf::Clock deltaClock;
	cc::TileInfo::Init();
	cc::EntityInfo::Init();
	cc::EntityInfo::Build();
	auto &io = ImGui::GetIO();
	io.Fonts->Clear();
	io.Fonts->AddFontFromFileTTF("content/resources/fonts/default font.ttf", 20.f);
	if (!ImGui::SFML::UpdateFontTexture())
	{
		window->close();
	}
	macro.active = false;
	while (window->isOpen())
	{
		InputState inputState = input.ProcessEvents(*window);
		auto time = deltaClock.restart();
		double dt = time.asSeconds();
		ImGui::SFML::Update(*window, time);
		inputState.DrawToWindow();
		if (macro.active)
		{
			macro.Execute(state.get());
			InputState copy = InputState(macro.inputState);
			inputState = copy;
		}
		state->Update(inputState, dt);
		window->clear(sf::Color(0, 0, 0));
		state->Render();
		ImGui::SFML::Render(*window);

		window->display();
	}
	if (server)
		server->Stop();
	return 0;
}