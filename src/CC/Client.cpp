#include "Client.hpp"
#include "../Main.hpp"
#include "SaveManager.hpp"
#include "MainMenu.hpp"
#include "CSMessage.hpp"
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
		planets[activePlanet]->VisibleUpdate(renderTarget,inputState,deltaTime);
		// for (auto &p : planets)
		// {
		// 	p->VisibleUpdate(renderTarget, inputState, deltaTime);
		// }
		if (inputState.Pressed(sf::Keyboard::Key::Escape))
		{
			paused = !paused;
		}
		if (paused)
		{
			DisplayPauseMenu();
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
				state->Update(inputState, 0);
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
			}
		}

		ImGui::End();
	}

	void Client::ProcessPacket(sf::Packet &packet)
	{
		uint16_t t;
		packet >> t;
		std::cout << "received message of type " << std::to_string(t) << std::endl;
		CSMessageType type = (CSMessageType)t;
		if (type == CSMessageType::JOIN_DATA)
		{
			LoadJoinData(packet);
		}
	}

	void Client::SendPacket(sf::Packet &packet)
	{
		// if (server.get() != nullptr)
		// {
		// 	server->HandlePacket(id, packet);
		// 	return;
		// }
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
		// --- Read list of current players ---

		// NOTE: `packet << data.size()` on the sending side pushes a std::size_t,
		// which on a 64-bit build resolves to sf::Packet's Uint64 overload.
		// Reading it back needs a matching type or the stream will misalign.
		uint64_t playerDataSize;
		packet >> playerDataSize;

		// sf::Packet has no built-in "extract N raw bytes mid-stream" call,
		// so pull them out one byte at a time via the Uint8 overload.
		std::vector<uint8_t> playerData;
		playerData.reserve(playerDataSize);
		for (uint64_t i = 0; i < playerDataSize; i++)
		{
			uint8_t byte;
			packet >> byte;
			playerData.push_back(byte);
		}

		Serializer s(Serializer::Mode::READ, Serializer::Format::BINARY, {}, playerData);

		int n;
		s.field("n", n);
		for (int i = 0; i < n; i++)
		{
			PlayerData p;
			s.field(std::to_string(i), p);
			if (p.username == SaveManager::username)
			{
				activePlanet = p.planet;
				planets[activePlanet]->camera.position = p.cameraPosition;
				planets[activePlanet]->camera.targetZoom = p.cameraZoom;
				continue;
			}
			otherPlayers.push_back(p);
		}

		// --- Read chunk data ---

		int numChunks;
		packet >> numChunks;

		for (int i = 0; i < numChunks; i++)
		{
			int posX, posY;
			packet >> posX >> posY;

			uint64_t chunkByteSize; // same Uint64 assumption as above
			packet >> chunkByteSize;

			std::vector<uint8_t> chunkData;
			chunkData.reserve(chunkByteSize);
			for (uint64_t j = 0; j < chunkByteSize; j++)
			{
				uint8_t byte;
				packet >> byte;
				chunkData.push_back(byte);
			}
			Chunk *c = new Chunk({posX, posY});
			c->LoadByteData(chunkData);
			planets[activePlanet]->chunks[{posX,posY}] = std::unique_ptr<Chunk>(c);
			// GUESS: not sure how Chunk is constructed/inserted client-side.
			// Something like:
			// auto chunk = std::make_unique<Chunk>();
			// chunk->position = {posX, posY};
			// chunk->LoadFromByteData(chunkData); // mirror of GetByteData()
			// planets[activePlanet]->chunks[{posX, posY}] = std::move(chunk);
		}
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
		std::cout << "CONNECTED, SENDING USERNAME" << std::endl;
		SendUsername();
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
				std::cout << "RECEIVED PACKET" << std::endl;
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
	void Client::SendUsername()
	{
		sf::Packet packet;
		packet << (uint16_t)CSMessageType::SEND_USERNAME;
		packet << SaveManager::username;
		SendPacket(packet);
	}
}