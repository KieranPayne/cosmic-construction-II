#pragma once
#include "../PCH.hpp"
#include "Planet.hpp"
#include "../State.hpp"
#include "PlayerData.hpp"
namespace cc
{
    class Client : public Kosmic::State
    {
        private:
        bool chatLogScrollToBottom = false;
        bool paused = false;
        bool connected = false;
        sf::TcpSocket socket;
        std::vector<PlayerData> otherPlayers = {};
        std::vector<std::unique_ptr<Planet>> planets = {};
        std::vector<std::string> chatLog = {};
        int activePlanet;
        void DisplayPauseMenu();
        public:
        void AddPlanet(Planet* planet);
        void DerivedUpdate();
        void DerivedRender();
        Client(sf::RenderTarget* target);
        bool ConnectToServer(sf::IpAddress& ip, unsigned short port);
        void SendPacket(sf::Packet& packet);
        private:
        //NETWORKING STUFF
        void ProcessPacket(sf::Packet& packet);
        void LoadJoinData(sf::Packet& packet);
        void NewPlayerJoined(sf::Packet& packet);
        bool IsConnected()
        {
            return connected;
        }
        void OnServerClosed();
        void ReceivePackets();
        void SendUsername();
        enum class MessageOrigin
        {
            SELF,
            SERVER,
            PLAYER
        };
        char chatInput[256] = "";
        void SendChatMessage(const std::string& text);
        void LogMessage(std::string message, MessageOrigin origin = MessageOrigin::SELF, std::string username = "");
        void DrawLogWindow();
        void LoadChunks(sf::Packet& packet);
        void DrawOtherPlayers();
    };
}