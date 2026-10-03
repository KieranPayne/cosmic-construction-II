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
        bool loadedJoinData = false;
        sf::TcpSocket socket;
        std::vector<PlayerData> otherPlayers = {};
        std::vector<PlayerData> prevOtherPlayers = {};
        std::vector<sf::Clock> prevOtherClocks = {};
        std::vector<std::unique_ptr<Planet>> planets = {};
        std::vector<std::string> chatLog = {};
        int activePlanet = 0;
        sf::Clock sendPlayerDataClock;
        float timePerPlayerDataUpdate = 0.2f;
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
        public:
        enum class MessageOrigin
        {
            SELF,
            SERVER,
            PLAYER
        };
        char chatInput[256] = "";
        void LogMessage(std::string message, MessageOrigin origin = MessageOrigin::SELF, std::string username = "");
        private:
        void SendChatMessage(const std::string& text);
        void DrawLogWindow();
        void LoadChunks(sf::Packet& packet);
        void DrawOtherPlayers();
        PlayerData GetPlayerData();
    };
}