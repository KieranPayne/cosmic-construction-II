#include "Client.hpp"
#include "../Main.hpp"
#include "SaveManager.hpp"
#include "MainMenu.hpp"
namespace cc
{
	Client::Client(sf::RenderTarget *target)
	{
		this->renderTarget = target;
		activePlanet = 0;
		planets.push_back(std::make_unique<Planet>());
	}
	void Client::DerivedUpdate()
	{
		ReceivePackets();
		// for (auto &p : planets)
		// {
		// 	p->VisibleUpdate(renderTarget, inputState, deltaTime);
		// }
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
				state->Update(inputState, 0);
				delete this;
			}
		}
		else
		{
			if (ImGui::Button("disconnect"))
			{
				state = std::unique_ptr<Kosmic::State>(new MainMenu());
				InputState inputState;
				state->renderTarget = renderTarget;
				state->Update(inputState, 0);
				delete this;
			}
		}

		ImGui::End();
	}

	void Client::ProcessPacket(sf::Packet &packet)
	{
	}

	void Client::SendPacket(sf::Packet &packet)
	{
		if (server.get() != nullptr)
		{
			server->HandlePacket(id, packet);
			return;
		}
		if (!IsConnected())
		{
			return;
		}
		sf::Socket::Status status = socket.send(packet);

		if (status == sf::Socket::Status::Disconnected)
		{
			connected = false;
		}
	}

	void Client::LoadJoinData(sf::Packet &packet)
	{
	}

	bool Client::ConnectToServer(sf::IpAddress &ip, unsigned short port)
	{
		// If we're already connected, don't reconnect.
		if (connected)
			return true;

		// Temporarily make the socket blocking so connect() can
		// actually wait for the connection to be established.
		socket.setBlocking(true);

		if (socket.connect(ip, port) != sf::Socket::Status::Done)
		{
			socket.setBlocking(false);
			connected = false;
			return false;
		}

		socket.setBlocking(false);
		connected = true;
		std::cout << "CONNECTED" << std::endl;

		return true;
	}

	void Client::ReceivePackets()
	{
		if (!connected)
			return;

		while (true)
		{
			sf::Packet packet;

			sf::Socket::Status status = socket.receive(packet);

			if (status == sf::Socket::Status::Done)
			{
				ProcessPacket(packet);
			}
			else if (status == sf::Socket::Status::NotReady)
			{
				// No more packets currently waiting.
				break;
			}
			else if (status == sf::Socket::Status::Disconnected)
			{
				connected = false;
				break;
			}
			else
			{
				// Error
				connected = false;
				break;
			}
		}
	}
}