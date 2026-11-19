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
		world = std::make_unique<World>();
		world->client = this;
		for (auto &p : world->planets)
		{
			p->client = this;
		}
		knowledgeGraph = std::make_unique<KnowledgeGraph>(renderTarget, knowledgeDatabase);
	}

	void Client::Tick()
	{
		world->Tick();
	}
	void Client::DerivedUpdate()
	{
		ReceivePackets();
		FlushOutgoing();
		if (!connected || !loadedJoinData)
		{
			return;
		}
		while (serverTickCounter > world->currentTick)
		{
			if (queuedInputs.contains(world->currentTick))
			{
				for (int i = 0; i < queuedInputs[world->currentTick].size(); i++)
				{
					world->EnactInputBundle(queuedInputs[world->currentTick][i]);
				}
				queuedInputs.erase(world->currentTick);
			}
			PlayerData currPlayerData = GetPlayerData();
			Serializer s(Serializer::Mode::WRITE, Serializer::Format::BINARY);
			Serializer s2(Serializer::Mode::WRITE, Serializer::Format::BINARY);
			currPlayerData.Serialize(s);
			prevPlayerData.Serialize(s2);
			auto data = s.binary();
			auto data2 = s2.binary();
			if (data != data2)
			{
				currInputBundle.playerData = currPlayerData;
			}
			prevPlayerData = currPlayerData;
			SendInputBundle();
			Tick();
			currInputBundle = InputBundle();
			// currInputBundle.playerData = GetPlayerData();
			if (world->currentTick % 60 == 0)
			{
				sf::Packet p;
				p << (uint16_t)CSMessageType::STATE_CHECK;
				world->GenerateStateCheck(p);
				SendPacket(p);
			}
		}
		// std::cout << "on tick " << tickCounter << std::endl;
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
			world->planets[activePlanet]->VisibleUpdate(renderTarget, inputState, deltaTime);
			DrawLogWindow();
		}

		// debug shortcut: place a test image made of tiles
		if (inputState.Pressed(sf::Keyboard::Key::I))
		{
			std::string path = "content/resources/images/herbert.png";
			world->planets[activePlanet]->MakeImageFromTiles(path, {30, 30}, 150);
		}
	}

	void Client::DerivedRender()
	{
		if (!loadedJoinData || !connected)
		{
			return;
		}
		if (showingKnowledgeGraph)
		{
			knowledgeGraph->Render(inputState);
		}
		else
		{
			sf::View original = renderTarget->getView();
			world->planets[activePlanet]->camera.SetView(renderTarget);
			world->planets[activePlanet]->Render(renderTarget);
			DrawOtherPlayers();
			renderTarget->setView(original);
		}
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
		if (!loadedJoinData)
		{
			sf::Packet peek(packet);
			uint16_t peekType;
			peek >> peekType;
			if ((CSMessageType)peekType != CSMessageType::JOIN_DATA)
			{
				packetBuffer.push_back(packet);
				return;
			}
		}

		static int bytesReceived = 0;
		static sf::Clock byteTimer;
		if (!byteTimer.isRunning())
		{
			byteTimer.start();
		}
		bytesReceived += packet.getDataSize();
		std::cout << "Received: " << (int)(bytesReceived / 1024.f) << " per second: " << (int)(bytesReceived / 1024.f / byteTimer.getElapsedTime().asSeconds()) << std::endl;
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
			LogMessage("Player " + username + " disconnected.",ChatMessageOrigin::SERVER);
			RemoveOtherPlayer(username);
			break;
		}

		case CSMessageType::SERVER_MESSAGE:
		{
			std::string message;
			packet >> message;
			LogMessage(message, ChatMessageOrigin::SERVER);
			break;
		}

		// case CSMessageType::CHAT_MESSAGE:
		// {
		// 	std::string username, message;
		// 	packet >> username >> message;
		// 	LogMessage(message, ChatMessageOrigin::PLAYER, username);
		// 	break;
		// }

		case CSMessageType::BROADCAST_NEXT_TICK:
		{
			packet >> serverTickCounter;
			uint64_t numBundles;
			packet >> numBundles;
			// queuedInputs[serverTickCounter] = {};
			for (int i = 0; i < numBundles; i++)
			{
				auto data = ReadBytesFromPacket(packet);
				Serializer s(Serializer::Mode::READ, Serializer::Format::BINARY, {}, data);
				InputBundle in;
				in.Serialize(s);
				queuedInputs[serverTickCounter].push_back(std::move(in));
				// world->EnactInputBundle(in);
			}
			if (!tickTimer.isRunning())
			{
				tickTimer.start();
			}
			else
			{
				timesBetweenTicks.push_back(tickTimer.getElapsedTime().asSeconds() / (serverTickCounter - world->currentTick));
				if (timesBetweenTicks.size() > 60)
				{
					timesBetweenTicks.erase(timesBetweenTicks.begin());
				}
				tickTimer.restart();
				if (timesBetweenTicks.size() >= 1 && serverTickCounter % 60 == 0)
				{
					double total = 0;
					for (auto t : timesBetweenTicks)
					{
						total += t;
					}
					total /= (double)timesBetweenTicks.size();
					std::cout << "avg time between ticks: " << total << " tps: " << (1.0 / total) << std::endl;
				}
			}
			break;
		}
		// case CSMessageType::UPDATE_KNOWLEDGE_DB:
		// {
		// 	LoadKnowledgeDB(packet);
		// }
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
		world->LoadJoinData(packet);
		for (auto &p : world->planets)
		{
			p->client = this;
		}
		serverTickCounter = world->currentTick;
		std::string msg = "Join data processed. Other players:";
		for (auto &p : world->players)
		{
			if (p.username == SaveManager::username)
			{
				continue;
			}
			msg += "\n" + p.username;
		}
		LogMessage(msg,ChatMessageOrigin::SERVER);
		for (auto &p : packetBuffer)
		{
			ProcessPacket(p);
		}
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
		LogMessage("Connected, sending username",ChatMessageOrigin::SERVER);
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
		AddPlayer(p);
		LogMessage("Player Connected: " + p.username, ChatMessageOrigin::SERVER);
	}

	void Client::AddPlayer(const PlayerData &player)
	{
		world->AddPlayer(player);
	}

	void Client::RemoveOtherPlayer(const std::string &username)
	{
		for (size_t i = 0; i < world->players.size(); i++)
		{
			if (world->players[i].username == username)
			{
				world->SavePlayer(i);
				world->players.erase(world->players.begin() + i);
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

	void Client::LogMessage(std::string message, ChatMessageOrigin origin, std::string username)
	{
		// if (origin == ChatMessageOrigin::SELF)
		// {
		// 	chatLog.push_back("> " + message);
		// }
		if (origin == ChatMessageOrigin::SERVER)
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
		currInputBundle.chatMessages.push_back(ChatMessage(ChatMessageOrigin::PLAYER,text,SaveManager::username));
		// if (!text.empty() && text[0] == '/')
		// 	LogMessage(text);
		// else
		// LogMessage(text, ChatMessageOrigin::PLAYER, SaveManager::username);
		// sf::Packet p;
		// p << (uint16_t)CSMessageType::CHAT_MESSAGE;
		// p << text;
		// SendPacket(p);
	}

	// void Client::LoadChunks(sf::Packet &packet)
	// {
	// 	Planet &planet = *planets[activePlanet];
	// 	uint64_t n;
	// 	packet >> n;
	// 	for (uint64_t i = 0; i < n; i++)
	// 	{
	// 		int posX, posY;
	// 		packet >> posX >> posY;
	// 		// always read the chunk's bytes, even when skipping it, so the rest of the packet stays aligned
	// 		auto chunkData = ReadBytesFromPacket(packet);
	// 		if (planet.chunks.contains({posX, posY}))
	// 			continue;
	// 		Chunk *c = new Chunk({posX, posY});
	// 		c->LoadByteData(chunkData);
	// 		planet.chunks[{posX, posY}] = std::unique_ptr<Chunk>(c);
	// 	}
	// }

	void Client::DrawOtherPlayers()
	{
		sf::RectangleShape rect;
		for (size_t i = 0; i < world->players.size(); i++)
		{
			PlayerData &p = world->players[i];
			if (p.username == SaveManager::username)
			{
				continue;
			}
			if (p.planet != activePlanet)
			{
				continue;
			}

			// how far between their previous and latest update we are, from 0 to 1
			// float t = std::clamp(prevOtherClocks[i].getElapsedTime().asSeconds() / timePerPlayerDataUpdate, 0.f, 1.f);
			sf::Color col = UsernameToColor(p.username);
			sf::Vector2f targetResolution = p.resolution;
			float zoom = p.cameraZoom;
			sf::Vector2f position = p.cameraPosition;

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
		p.cameraPosition = world->planets[activePlanet]->camera.position;
		p.cameraZoom = world->planets[activePlanet]->camera.zoom;
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
	// void Client::SendKnowledgeDB()
	// {
	// 	sf::Packet packet;
	// 	packet << (uint16_t)CSMessageType::REQUEST_UPDATE_KNOWLEDGE_DB;
	// 	Serializer s(Serializer::Mode::WRITE, Serializer::Format::BINARY);
	// 	knowledgeDatabase.Serialize(s);
	// 	AppendSerializerToPacket(packet, s);
	// 	SendPacket(packet);
	// }
	// void Client::LoadKnowledgeDB(sf::Packet &packet)
	// {
	// 	Serializer s = ReadSerializerFromPacket(packet);
	// 	knowledgeDatabase.Serialize(s);
	// }

	void Client::SendInputBundle()
	{
		bool playerDatSame = false;
		PlayerData currPlayerData = GetPlayerData();
		{
			Serializer s(Serializer::Mode::WRITE, Serializer::Format::BINARY);
			Serializer s2(Serializer::Mode::WRITE, Serializer::Format::BINARY);
			currPlayerData.Serialize(s);
			prevPlayerData.Serialize(s2);
			auto data = s.binary();
			auto data2 = s2.binary();
			if (data == data2)
			{
				playerDatSame = true;
			}
			prevPlayerData = currPlayerData;
		}
		{
			InputBundle defaultBundle;
			Serializer s(Serializer::Mode::WRITE, Serializer::Format::BINARY);
			Serializer s2(Serializer::Mode::WRITE, Serializer::Format::BINARY);
			defaultBundle.Serialize(s);
			currInputBundle.Serialize(s2);
			auto data = s.binary();
			auto data2 = s2.binary();
			if (data == data2 && playerDatSame)
			{
				return;
			}else
			{
				currInputBundle.playerData = currPlayerData;
			}
		}
		
		sf::Packet p;
		p << (uint16_t)CSMessageType::CLIENT_INPUT_BUNDLE;
		p << world->currentTick;
		Serializer s(Serializer::Mode::WRITE, Serializer::Format::BINARY);
		currInputBundle.Serialize(s);
		auto data = s.binary();
		p << (uint64_t)data.size();
		p.append(data.data(), data.size());
		SendPacket(p);
	}

	void Client::ProcessTickBroadcast(sf::Packet &p)
	{
		p >> serverTickCounter;
		std::vector<InputBundle> inputBundles;
		uint64_t numInputBundles;
		p >> numInputBundles;
		for (int i = 0; i < numInputBundles; i++)
		{
			auto data = ReadBytesFromPacket(p);
			Serializer s(Serializer::Mode::READ, Serializer::Format::BINARY, {}, data);
			InputBundle in;
			in.Serialize(s);
			inputBundles.push_back(std::move(in));
		}
		// ensure any tile changes etc. are applied in the same order by everyone
		std::sort(inputBundles.begin(), inputBundles.end(),
				  [](const InputBundle &a, const InputBundle &b)
				  {
					  return a.clientId < b.clientId;
				  });
		for (auto &in : inputBundles)
		{
			world->EnactInputBundle(in);
		}
	}

}