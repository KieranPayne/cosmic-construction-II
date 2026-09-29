#include "Client.hpp"
#include "../Main.hpp"
#include "SaveManager.hpp"
#include "MainMenu.hpp"
#include "CSMessage.hpp"
#include "Utils.hpp"
namespace cc
{
	Client::Client(sf::RenderTarget *target)
	{
		this->renderTarget = target;
		activePlanet = 0;
		AddPlanet(new Planet());
	}
	void Client::AddPlanet(Planet* planet)
	{
		planet->client = this;
		planets.push_back(std::unique_ptr<Planet>(planet));
	}
	void Client::DerivedUpdate()
	{
		ReceivePackets();
		if (!connected)
		{
			return;
		}
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
		else
		{
			planets[activePlanet]->VisibleUpdate(renderTarget, inputState, deltaTime);
			auto chunks = planets[activePlanet]->GetChunksToRequest(renderTarget);
			if (chunks.size() > 0)
			{
				sf::Packet p;
				p << (uint16_t)CSMessageType::REQUEST_CHUNKS;
				p << (uint64_t)chunks.size();
				for (auto& c : chunks)
				{
					p << c.x << c.y;
				}
				SendPacket(p);
			}
			DrawLogWindow();
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
				server.reset(nullptr);
				state = std::unique_ptr<Kosmic::State>(new MainMenu(renderTarget));
				InputState inputState;
				state->Update(inputState, 0);
			}
		}
		else
		{
			if (ImGui::Button("disconnect"))
			{
				state = std::unique_ptr<Kosmic::State>(new MainMenu(renderTarget));
				InputState inputState;
				state->Update(inputState, 0);
			}
		}

		ImGui::End();
	}

	void Client::ProcessPacket(sf::Packet &packet)
	{
		uint16_t t;
		packet >> t;
		// LogMessage("Received message of type " + std::to_string(t));
		CSMessageType type = (CSMessageType)t;
		if (type == CSMessageType::JOIN_DATA)
		{
			LoadJoinData(packet);
		}
		else if (type == CSMessageType::PLAYER_JOINED)
		{
			NewPlayerJoined(packet);
		}
		else if (type == CSMessageType::PLAYER_LEFT)
		{
			std::string username;
			packet >> username;
			LogMessage("Player " + username + " disconnected.");
			for (int i = 0; i < otherPlayers.size(); i++)
			{
				if (otherPlayers[i].username == username)
				{
					otherPlayers.erase(otherPlayers.begin() + i);
					break;
				}
			}
		}
		else if (type == CSMessageType::SERVER_MESSAGE)
		{
			std::string message;
			packet >> message;
			LogMessage(message, MessageOrigin::SERVER);
		}
		else if (type == CSMessageType::CHAT_MESSAGE)
		{
			std::string username, message;
			packet >> username >> message;
			LogMessage(message, MessageOrigin::PLAYER,username);
		}else if (type == CSMessageType::CHUNK_DATA)
		{
			LoadChunks(packet);
		}else if (type == CSMessageType::SET_TILE)
		{
			LogMessage("Setting tile");
			sf::Vector2i position;
            packet >> position.x >> position.y;
            uint16_t tileType;
            packet >> tileType;
            bool hasTileEntity;
            packet >> hasTileEntity;
            TileEntity* e = nullptr;
            if (hasTileEntity)
            {
                auto data = ReadBytesFromPacket(packet);
                Serializer s(Serializer::Mode::READ,Serializer::Format::BINARY,{},data);
                e = CreateTileEntityFromType(tileType);
                e->Serialize(s);
            }
            planets[activePlanet]->SetTileAt(position,Tile(tileType),e,true);
		}else if (type == CSMessageType::ADD_ENTITY)
		{
			Entity* e = LoadEntityFromPacket(packet);
            planets[activePlanet]->AddEntity(e, true);
		}else if (type == CSMessageType::UPDATE_ENTITY)
		{
			int index;
			packet >> index;
			Entity* e = LoadEntityFromPacket(packet);
			planets[activePlanet]->ReplaceEntity(index,e);
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
			OnServerClosed();
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
			auto chunkData = ReadBytesFromPacket(packet);
			// uint64_t chunkByteSize; // same Uint64 assumption as above
			// packet >> chunkByteSize;

			// std::vector<uint8_t> chunkData;
			// chunkData.reserve(chunkByteSize);
			// for (uint64_t j = 0; j < chunkByteSize; j++)
			// {
			// 	uint8_t byte;
			// 	packet >> byte;
			// 	chunkData.push_back(byte);
			// }
			Chunk *c = new Chunk({posX, posY});
			c->LoadByteData(chunkData);
			planets[activePlanet]->chunks[{posX, posY}] = std::unique_ptr<Chunk>(c);
		}
		//load entities;
		uint64_t numEntities;
		packet >> numEntities;
		for (int i = 0; i < numEntities; i ++)
		{
			Entity* e = LoadEntityFromPacket(packet);
			planets[activePlanet]->AddEntity(e);
		}

		std::string msg = "Join data processed. Other players:";
		for (auto &p : otherPlayers)
		{
			msg += "\n" + p.username;
		}
		LogMessage(msg);
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
		LogMessage("Connected, sending username");
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
				ProcessPacket(packet);
			}
			else if (status == sf::Socket::Status::NotReady)
			{
				// No more packets currently waiting.
				break;
			}
			else if (status == sf::Socket::Status::Disconnected)
			{
				OnServerClosed();
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
	void Client::NewPlayerJoined(sf::Packet &packet)
	{
		auto data = ReadBytesFromPacket(packet);
		// uint64_t n;
		// packet >> n;
		// // std::vector<uint8_t> data;
		// data.reserve(n);
		// for (int i = 0; i < n; i++)
		// {
		// 	uint8_t byte;
		// 	packet >> byte;
		// 	data.push_back(byte);
		// }
		Serializer s(Serializer::Mode::READ, Serializer::Format::BINARY, {}, data);
		PlayerData p;
		s.field("player", p);
		otherPlayers.push_back(p);
		LogMessage("Player Connected: " + p.username);
	}
	void Client::OnServerClosed()
	{
		connected = false;
		std::cout << "Server closed, returning to main menu" << std::endl;
		state = std::unique_ptr<Kosmic::State>(new MainMenu(renderTarget));
		InputState inputState;
		state->Update(inputState, 0);
	}
	void Client::LogMessage(std::string message, MessageOrigin origin, std::string username)
	{
		if (origin == MessageOrigin::SELF)
		{
			chatLog.push_back("> " + message);
		}
		else if (origin == MessageOrigin::SERVER)
		{
			chatLog.push_back("<SERVER> " + message);
		}
		else
		{
			chatLog.push_back("<" + username + "> " + message);
		}
		chatLogScrollToBottom = true;
	}
	void Client::DrawLogWindow()
	{
		ImGui::SetNextWindowPos({897, 3}, ImGuiCond_Once);
		ImGui::SetNextWindowSize({379, 189}, ImGuiCond_Once);
		ImGui::Begin("Chat Log");

		// Reserve space at the bottom for the separator + input row
		const float footerHeight = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();

		ImGui::BeginChild("ChatScrollRegion", ImVec2(0, -footerHeight), false);
		for (auto &m : chatLog)
		{
			ImGui::TextWrapped("%s", m.c_str());
		}
		if (chatLogScrollToBottom)
		{
			ImGui::SetScrollHereY(1.f);
			chatLogScrollToBottom = false;
		}
		ImGui::EndChild();

		ImGui::Separator();

		bool send = false;
		ImGui::SetNextItemWidth(-60);
		if (ImGui::InputText("##ChatInput", chatInput, sizeof(chatInput), ImGuiInputTextFlags_EnterReturnsTrue))
		{
			send = true;
			ImGui::SetKeyboardFocusHere(-1); // keep the field focused after pressing Enter
		}
		ImGui::SameLine();
		if (ImGui::Button("Send"))
		{
			send = true;
		}

		if (send && chatInput[0] != '\0')
		{
			SendChatMessage(chatInput);
			chatInput[0] = '\0';
		}

		ImGui::End();
	}
	void Client::SendChatMessage(const std::string& text)
	{
		LogMessage(text,MessageOrigin::PLAYER,SaveManager::username);
		sf::Packet p;
		p << (uint16_t)CSMessageType::CHAT_MESSAGE;
		p << text;
		SendPacket(p);
	}
	void Client::LoadChunks(sf::Packet& packet)
	{
		uint64_t n;
		packet >> n;
		for (int i = 0; i < n; i ++)
		{
			int posX, posY;
			packet >> posX >> posY;
			auto chunkData = ReadBytesFromPacket(packet);
			// uint64_t chunkByteSize; // same Uint64 assumption as above
			// packet >> chunkByteSize;

			// std::vector<uint8_t> chunkData;
			// chunkData.reserve(chunkByteSize);
			// for (uint64_t j = 0; j < chunkByteSize; j++)
			// {
			// 	uint8_t byte;
			// 	packet >> byte;
			// 	chunkData.push_back(byte);
			// }
			Chunk *c = new Chunk({posX, posY});
			c->LoadByteData(chunkData);
			planets[activePlanet]->chunks[{posX, posY}] = std::unique_ptr<Chunk>(c);
		}
	}


}