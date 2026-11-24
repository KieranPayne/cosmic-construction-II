#include "Client.hpp"
#include "../Main.hpp"
#include "SaveManager.hpp"
#include "MainMenu.hpp"
#include "CSMessage.hpp"
#include "Utils.hpp"

#ifdef _WIN32
#include <winsock2.h>
#else
#include <netinet/tcp.h>
#include <sys/socket.h>
#endif

namespace
{
	sf::SocketHandle GetNativeHandle(const sf::Socket &socket)
	{
		struct Accessor : sf::Socket
		{
			static sf::SocketHandle Get(const sf::Socket &s)
			{
				return (s.*(&Accessor::getNativeHandle))();
			}
		};
		return Accessor::Get(socket);
	}

	void DisableNagle(sf::TcpSocket &socket)
	{
		int flag = 1;
		setsockopt(
			GetNativeHandle(socket),
			IPPROTO_TCP,
			TCP_NODELAY,
			reinterpret_cast<const char *>(&flag),
			sizeof(flag));
	}
}

namespace cc
{
	Client::Client(sf::RenderTarget *target)
	{
		this->renderTarget = target;
		activePlanet = 0;
		world = std::make_unique<World>();
		world->client = this;
		// for (auto &p : world->planets)
		// {
		// 	p->client = this;
		// }
		knowledgeGraph = std::make_unique<KnowledgeGraph>(renderTarget, world->knowledgeDatabase);
	}

	void Client::Tick()
	{
		world->Tick();
	}

	void Client::StepSimulationTick()
	{
		if (queuedInputs.contains(world->currentTick))
		{
			for (size_t i = 0; i < queuedInputs[world->currentTick].size(); i++)
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

		if (world->currentTick % 60 == 0)
		{
			sf::Packet p;
			p << (uint16_t)CSMessageType::STATE_CHECK;
			world->GenerateStateCheck(p);
			SendPacket(p);
		}
		FlushOutgoing();
	}

	void Client::DerivedUpdate()
	{
		ReceivePackets();
		FlushOutgoing();
		if (!connected || !loadedJoinData)
		{
			return;
		}
		if (knowledgeGraph->ConsumeDatabaseDirty())
		{
			currInputBundle.knowledgeDatabaseChanged = std::make_unique<KnowledgeDatabase>(world->knowledgeDatabase);
		}

		const double tickInterval = 1.0 / static_cast<double>(std::max(1.f, TPS));
		const uint64_t targetBufferTicks = (server != nullptr) ? 1 : 2;
		uint64_t bufferedTicks = (serverTickCounter > world->currentTick)
									 ? (serverTickCounter - world->currentTick)
									 : 0;

		if (bufferingTicks)
		{
			if (bufferedTicks >= targetBufferTicks)
			{
				bufferingTicks = false;
				simAccumulator = 0.0;
			}
		}

		if (!bufferingTicks)
		{
			// Slightly speed up playback if packets backed up after a lag spike
			const double timeScale = (bufferedTicks > targetBufferTicks + 2) ? 1.15 : 1.0;
			const double clampedDt = std::min(deltaTime, tickInterval * 5.0);
			simAccumulator += clampedDt * timeScale;

			while (simAccumulator >= tickInterval)
			{
				if (serverTickCounter <= world->currentTick)
				{
					// Next tick hasn't arrived yet: hold cleanly at the end of the current tick
					// so renderAlpha stays at 1.0 instead of snapping back to 0.0.
					simAccumulator = tickInterval;
					bufferingTicks = true;
					break;
				}

				StepSimulationTick();
				simAccumulator -= tickInterval;
			}
		}

		renderAlpha = bufferingTicks
						  ? 1.0f
						  : std::clamp(static_cast<float>(simAccumulator / tickInterval), 0.0f, 1.0f);

		if (inputState.Pressed(sf::Keyboard::Key::Escape))
		{
			paused = !paused;
		}

		if (!ImGui::GetIO().WantCaptureKeyboard && inputState.Pressed(sf::Keyboard::Key::K))
		{
			showingKnowledgeGraph = !showingKnowledgeGraph;
			if (!showingKnowledgeGraph && SaveManager::debuggingKnowledgeGraph)
				knowledgeGraph->SaveToFile();
		}

		if (paused)
		{
			DisplayPauseMenu();
		}
		else if (showingKnowledgeGraph)
		{
			knowledgeGraph->Update(deltaTime, inputState);
			DrawLogWindow();
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
			world->planets[activePlanet]->Render(renderTarget, renderAlpha);
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
			LogMessage("Player " + username + " disconnected.", ChatMessageOrigin::SERVER);
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

		case CSMessageType::BROADCAST_NEXT_TICK:
		{
			packet >> serverTickCounter;
			uint64_t numBundles;
			packet >> numBundles;
			for (uint64_t i = 0; i < numBundles; i++)
			{
				auto data = ReadBytesFromPacket(packet);
				Serializer s(Serializer::Mode::READ, Serializer::Format::BINARY, {}, data);
				InputBundle in;
				in.Serialize(s);
				queuedInputs[serverTickCounter].push_back(std::move(in));
			}
			if (!tickTimer.isRunning())
			{
				tickTimer.start();
			}
			else if (serverTickCounter > world->currentTick)
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
					for (auto elapsed : timesBetweenTicks)
					{
						total += elapsed;
					}
					total /= (double)timesBetweenTicks.size();
					std::cout << "avg time between ticks: " << total << " tps: " << (1.0 / total) << std::endl;
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
		world->LoadJoinData(packet);
		knowledgeGraph->LoadFromFile();

		serverTickCounter = world->currentTick;
		bufferingTicks = true;
		simAccumulator = 0.0;
		renderAlpha = 1.0f;

		std::string msg = "Join data processed. Other players:";
		for (auto &p : world->players)
		{
			if (p.username == SaveManager::username)
			{
				activePlanet = p.planet;
				continue;
			}
			msg += "\n" + p.username;
		}
		LogMessage(msg, ChatMessageOrigin::SERVER);
		for (auto &p : packetBuffer)
		{
			ProcessPacket(p);
		}
		packetBuffer.clear();
	}

	bool Client::ConnectToServer(sf::IpAddress &ip, unsigned short port)
	{
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
		DisableNagle(socket);
		connected = true;
		LogMessage("Connected, sending username", ChatMessageOrigin::SERVER);
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
				break;
			}
			else if (status == sf::Socket::Status::Disconnected)
			{
				OnServerClosed();
				break;
			}
			else
			{
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
		currInputBundle.chatMessages.push_back(ChatMessage(ChatMessageOrigin::PLAYER, text, SaveManager::username));
	}

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
				OnServerClosed();
				return;
			}
			else
				break;
		}
	}

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
			}
			else
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
		for (uint64_t i = 0; i < numInputBundles; i++)
		{
			auto data = ReadBytesFromPacket(p);
			Serializer s(Serializer::Mode::READ, Serializer::Format::BINARY, {}, data);
			InputBundle in;
			in.Serialize(s);
			inputBundles.push_back(std::move(in));
		}
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

	void Client::OnKnowledgeDatabaseUpdated()
	{
		if (knowledgeGraph)
			knowledgeGraph->SyncWithDatabase();
	}
}