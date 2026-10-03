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
		sendPlayerDataClock.start();
	}

	void Client::AddPlanet(Planet *planet)
	{
		planet->client = this;
		planets.push_back(std::unique_ptr<Planet>(planet));
	}

	void Client::DerivedUpdate()
	{
		ReceivePackets();
		FlushOutgoing();
		if (!connected || !loadedJoinData)
		{
			return;
		}

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
			for (auto &p : planets)
			{
				p->Update(deltaTime);
			}
			planets[activePlanet]->VisibleUpdate(renderTarget, inputState, deltaTime);

			// ask the server for any chunks that are in view but not loaded yet
			auto chunks = planets[activePlanet]->GetChunksToRequest(renderTarget);
			if (chunks.size() > 0)
			{
				sf::Packet p;
				p << (uint16_t)CSMessageType::REQUEST_CHUNKS;
				p << (uint64_t)chunks.size();
				for (auto &c : chunks)
				{
					p << c.x << c.y;
				}
				SendPacket(p);
			}

			DrawLogWindow();

			// periodically tell the server where this player is looking
			if (sendPlayerDataClock.getElapsedTime().asSeconds() > timePerPlayerDataUpdate)
			{
				sendPlayerDataClock.restart();
				sf::Packet p;
				p << (uint16_t)CSMessageType::UPDATE_PLAYER_DATA;
				Serializer s(Serializer::Mode::WRITE, Serializer::Format::BINARY);
				GetPlayerData().Serialize(s);
				auto data = s.binary();
				p << (uint64_t)data.size();
				p.append(data.data(), data.size());
				SendPacket(p);
			}
		}

		// debug shortcut: place a test image made of tiles
		if (inputState.Pressed(sf::Keyboard::Key::I))
		{
			std::string path = "content/resources/images/borzoi.png";
			planets[activePlanet]->MakeImageFromTiles(path, {30, 30}, 150);
		}
	}

	void Client::DerivedRender()
	{
		if (!loadedJoinData || !connected)
		{
			return;
		}
		sf::View original = renderTarget->getView();
		planets[activePlanet]->camera.SetView(renderTarget);
		planets[activePlanet]->Render(renderTarget);
		DrawOtherPlayers();
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
			// this client is hosting the server
			if (ImGui::Button("save and quit"))
			{
				server->Stop();
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
		switch ((CSMessageType)t)
		{
		case CSMessageType::JOIN_DATA:
			LoadJoinData(packet);
			break;

		case CSMessageType::PLAYER_JOINED:
			NewPlayerJoined(packet);
			break;

		case CSMessageType::PLAYER_LEFT:
		{
			std::string username;
			packet >> username;
			LogMessage("Player " + username + " disconnected.");
			RemoveOtherPlayer(username);
			break;
		}

		case CSMessageType::SERVER_MESSAGE:
		{
			std::string message;
			packet >> message;
			LogMessage(message, MessageOrigin::SERVER);
			break;
		}

		case CSMessageType::CHAT_MESSAGE:
		{
			std::string username, message;
			packet >> username >> message;
			LogMessage(message, MessageOrigin::PLAYER, username);
			break;
		}

		case CSMessageType::CHUNK_DATA:
			LoadChunks(packet);
			break;

		case CSMessageType::SET_TILES:
		{
			int n;
			packet >> n;
			for (int i = 0; i < n; i++)
			{
				sf::Vector2i position;
				uint16_t tileType;
				bool hasTileEntity;
				packet >> position.x >> position.y >> tileType >> hasTileEntity;
				TileEntity *e = nullptr;
				if (hasTileEntity)
				{
					auto data = ReadBytesFromPacket(packet);
					Serializer s(Serializer::Mode::READ, Serializer::Format::BINARY, {}, data);
					e = CreateTileEntityFromType(tileType);
					e->Serialize(s);
				}
				planets[activePlanet]->SetTileAt(position, Tile(tileType), e, true);
			}
			break;
		}

		case CSMessageType::ADD_ENTITIES:
		{
			int n;
			packet >> n;
			for (int i = 0; i < n; i++)
			{
				Entity *e = LoadEntityFromPacket(packet);
				planets[activePlanet]->AddEntity(e, true);
			}
			break;
		}

		case CSMessageType::UPDATE_ENTITIES:
		{
			int n;
			packet >> n;
			for (int i = 0; i < n; i++)
			{
				int index;
				packet >> index;
				Entity *e = LoadEntityFromPacket(packet);
				planets[activePlanet]->ReplaceEntity(index, e);
			}
			break;
		}

		case CSMessageType::UPDATE_PLAYER_DATA:
		{
			std::string username;
			packet >> username;
			auto data = ReadBytesFromPacket(packet);
			Serializer s(Serializer::Mode::READ, Serializer::Format::BINARY, {}, data);
			for (size_t i = 0; i < otherPlayers.size(); i++)
			{
				if (otherPlayers[i].username == username)
				{
					prevOtherPlayers[i] = otherPlayers[i];
					prevOtherClocks[i].restart();
					otherPlayers[i].Serialize(s);
				}
			}
			break;
		}

		default:
			break;
		}
	}

	void Client::SendPacket(sf::Packet &packet)
	{
		if (!IsConnected())
		{
			return;
		}
		outgoingPackets.push_back(packet);
	}

	void Client::LoadJoinData(sf::Packet &packet)
	{
		loadedJoinData = true;

		// players currently on the server, including this one
		auto playerData = ReadBytesFromPacket(packet);
		Serializer s(Serializer::Mode::READ, Serializer::Format::BINARY, {}, playerData);

		int numPlayers;
		s.field("n", numPlayers);
		for (int i = 0; i < numPlayers; i++)
		{
			PlayerData p;
			s.field(std::to_string(i), p);
			if (p.username == SaveManager::username)
			{
				// this is us: restore our saved planet and camera
				activePlanet = p.planet;
				planets[activePlanet]->camera.position = p.cameraPosition;
				planets[activePlanet]->camera.targetZoom = p.cameraZoom;
				continue;
			}
			AddOtherPlayer(p);
		}

		Planet &planet = *planets[activePlanet];

		// chunks around the player
		int numChunks;
		packet >> numChunks;
		for (int i = 0; i < numChunks; i++)
		{
			int posX, posY;
			packet >> posX >> posY;
			auto chunkData = ReadBytesFromPacket(packet);
			Chunk *c = new Chunk({posX, posY});
			c->LoadByteData(chunkData);
			planet.chunks[{posX, posY}] = std::unique_ptr<Chunk>(c);
			planet.chunksRequested.emplace(sf::Vector2i{posX, posY});
		}

		// entities on the planet
		uint64_t numEntities;
		packet >> numEntities;
		for (uint64_t i = 0; i < numEntities; i++)
		{
			Entity *e = LoadEntityFromPacket(packet);
			planet.AddEntity(e, true);
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
		// if we're already connected, don't reconnect
		if (connected)
			return true;

		// the socket must be blocking so connect() waits for the connection to be established
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
				// no more packets waiting right now
				break;
			}
			else if (status == sf::Socket::Status::Disconnected)
			{
				OnServerClosed();
				break;
			}
			else
			{
				// socket error
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
		Serializer s(Serializer::Mode::READ, Serializer::Format::BINARY, {}, data);
		PlayerData p;
		s.field("player", p);
		AddOtherPlayer(p);
		LogMessage("Player Connected: " + p.username);
	}

	void Client::AddOtherPlayer(const PlayerData &player)
	{
		otherPlayers.push_back(player);
		prevOtherPlayers.push_back(player);
		prevOtherClocks.push_back(sf::Clock());
		prevOtherClocks.back().start();
	}

	void Client::RemoveOtherPlayer(const std::string &username)
	{
		for (size_t i = 0; i < otherPlayers.size(); i++)
		{
			if (otherPlayers[i].username == username)
			{
				otherPlayers.erase(otherPlayers.begin() + i);
				prevOtherPlayers.erase(prevOtherPlayers.begin() + i);
				prevOtherClocks.erase(prevOtherClocks.begin() + i);
				return;
			}
		}
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

		// reserve space at the bottom for the separator and input row
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
			// keep the field focused after pressing Enter
			ImGui::SetKeyboardFocusHere(-1);
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

	void Client::SendChatMessage(const std::string &text)
	{
		LogMessage(text, MessageOrigin::PLAYER, SaveManager::username);
		sf::Packet p;
		p << (uint16_t)CSMessageType::CHAT_MESSAGE;
		p << text;
		SendPacket(p);
	}

	void Client::LoadChunks(sf::Packet &packet)
	{
		Planet &planet = *planets[activePlanet];
		uint64_t n;
		packet >> n;
		for (uint64_t i = 0; i < n; i++)
		{
			int posX, posY;
			packet >> posX >> posY;
			// always read the chunk's bytes, even when skipping it, so the rest of the packet stays aligned
			auto chunkData = ReadBytesFromPacket(packet);
			if (planet.chunks.contains({posX, posY}))
				continue;
			Chunk *c = new Chunk({posX, posY});
			c->LoadByteData(chunkData);
			planet.chunks[{posX, posY}] = std::unique_ptr<Chunk>(c);
		}
	}

	void Client::DrawOtherPlayers()
	{
		sf::RectangleShape rect;
		for (size_t i = 0; i < otherPlayers.size(); i++)
		{
			PlayerData &p = otherPlayers[i];
			PlayerData &oldP = prevOtherPlayers[i];
			if (p.planet != activePlanet)
			{
				continue;
			}

			// how far between their previous and latest update we are, from 0 to 1
			float t = std::clamp(prevOtherClocks[i].getElapsedTime().asSeconds() / timePerPlayerDataUpdate, 0.f, 1.f);
			sf::Color col = UsernameToColor(p.username);
			sf::Vector2f targetResolution = Lerp(oldP.resolution, p.resolution, t);
			float zoom = Lerp(oldP.cameraZoom, p.cameraZoom, t);
			sf::Vector2f position = Lerp(oldP.cameraPosition, p.cameraPosition, t);

			rect.setFillColor(sf::Color::Transparent);
			rect.setOutlineColor(col);
			rect.setOutlineThickness(3.f);
			rect.setOrigin(targetResolution / 2.f * zoom);
			rect.setPosition(position);
			rect.setSize(zoom * targetResolution);
			renderTarget->draw(rect);
		}
	}

	PlayerData Client::GetPlayerData()
	{
		PlayerData p;
		p.username = SaveManager::username;
		p.cameraPosition = planets[activePlanet]->camera.position;
		p.cameraZoom = planets[activePlanet]->camera.targetZoom;
		p.planet = activePlanet;
		p.resolution = (sf::Vector2f)renderTarget->getSize();
		return p;
	}

	void Client::FlushOutgoing()
	{
		while (!outgoingPackets.empty())
		{
			auto status = socket.send(outgoingPackets.front());
			if (status == sf::Socket::Status::Done)
				outgoingPackets.pop_front();
			else if (status == sf::Socket::Status::Disconnected)
			{
				// OnServerClosed replaces the state, which destroys this client: touch nothing after it
				OnServerClosed();
				return;
			}
			else
				break;
		}
	}

}