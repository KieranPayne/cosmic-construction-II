#include "Client.hpp"
#include "../Main.hpp"
#include "SaveManager.hpp"
#include "MainMenu.hpp"
namespace cc
{
    Client::Client(sf::RenderTarget* target)
    {
        this->renderTarget = target;
        activePlanet = 0;
        planets.push_back(std::unique_ptr<Planet>());
    }
    void Client::DerivedUpdate()
    {
        for (auto& p : planets)
        {
            p->VisibleUpdate(renderTarget,inputState,deltaTime);
        }
    }
    void Client::DerivedRender()
    {
        sf::View original = renderTarget->getView();
		planets[activePlanet]->camera.SetView(renderTarget);
		planets[activePlanet]->Render(renderTarget);
		renderTarget->setView(original);
    }
    void Client::DisplayPauseMenu()
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
		if (server.get() != nullptr)
		{
			if (ImGui::Button("save and quit"))
			{
				SaveManager::SaveServer(server.get());
				state = std::unique_ptr<Kosmic::State>(new MainMenu());
				InputState inputState;
				state->renderTarget = renderTarget;
				state->Update(inputState,0);
				delete this;
			}
		}else
		{
			if (ImGui::Button("disconnect"))
			{
				state = std::unique_ptr<Kosmic::State>(new MainMenu());
				InputState inputState;
				state->renderTarget = renderTarget;
				state->Update(inputState,0);
				delete this;
			}
		}
		
		ImGui::End();
    }
}