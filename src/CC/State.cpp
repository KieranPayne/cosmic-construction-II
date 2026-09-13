#include "State.hpp"
#include "../Main.hpp"
#include "../Timer.hpp"
#include "../imgui/imgui.h"
#include "MainMenu.hpp"
#include "SaveManager.hpp"
namespace cc
{
	State::State()
	{
		seed = 0;
		activePlanet = 0;
		planets.push_back(std::make_unique<Planet>());
		planets[0]->index = 0;
		viewHeight = 15;
		paused = false;
		doTick = false;
		timeSinceTick = 0;
		timePerTick = 0.3;
	}
	void State::DerivedUpdate()
	{
		if (inputState.Pressed(sf::Keyboard::Key::Escape))
		{
			paused = !paused;
		}
		if (paused)
		{
			DisplayPauseMenu();
			return;
		}
		if (inputState.Pressed(sf::Keyboard::Key::Space))
		{
			doTick = !doTick;
			timeSinceTick = timePerTick;
		}

		if (doTick)
		{
			timeSinceTick += deltaTime;
			if (timeSinceTick > timePerTick)
			{
				timeSinceTick = 0;
				for (auto &p : planets)
				{
					p->Tick();
				}
			}
		}
		if (inputState.Pressed(sf::Keyboard::Key::K)){
			for (auto &p : planets)
			{
				p->Tick();
			}
		}

		for (auto &p : planets)
		{
			p->Update(deltaTime);
		}
		planets[activePlanet]->VisibleUpdate(renderTarget, inputState, deltaTime);
	}
	void State::DerivedRender()
	{
		sf::View original = renderTarget->getView();
		planets[activePlanet]->camera.SetView(renderTarget);
		planets[activePlanet]->Render(renderTarget);
		renderTarget->setView(original);
	}
	void State::DisplayPauseMenu()
	{
		ImGuiIO &io = ImGui::GetIO();
		ImVec2 displaySize = io.DisplaySize;

		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(displaySize);

		ImGui::Begin("PauseMenu", nullptr, windowFlags);
		if (ImGui::Button("resume"))
		{
			paused = false;
		}
		if (ImGui::Button("save and quit"))
		{
			SaveManager::Save(this);
			state = new MainMenu();
			InputState inputState;
			state->renderTarget = renderTarget;
			state->Update(inputState,0);
			delete this;
		}
		ImGui::End();
	}
	State::~State()
	{
	}
	void State::SetSeed(uint64_t seed)
	{
		this->seed = seed;
		planets[0]->SetSeed(seed);
	}
}