#pragma once
#include "../PCH.hpp"
#include "Planet.hpp"
#include "../State.hpp"
#include "PlayerData.hpp"
#include "KnowledgeGraph.hpp"
namespace cc
{
    /**
     * @brief The game state in which the player plays: connects to a Server over TCP, mirrors the part
     *        of the world it has received, draws it, and sends the player's actions back.
     *
     * The client works the same whether the server is on another machine or in this program.
     * Packets are received without blocking each frame, and outgoing packets are queued and sent
     * without blocking. Nothing is drawn until the server's join data has arrived.
     *
     * Uses `renderTarget`, `inputState` and `deltaTime`, which come from the Kosmic::State base class.
     */
    class Client : public Kosmic::State
    {
    private:
        /// Set by LogMessage() so that the chat window scrolls to the newest message the next time it is drawn.
        bool chatLogScrollToBottom = false;

        /// True while the pause menu is showing. While paused the world is not updated and no chunk requests
        /// or player data are sent, but incoming packets are still processed.
        bool paused = false;

        /// True while the connection to the server is open.
        bool connected = false;

        /// True once the server's JOIN_DATA packet has been processed. Updating and drawing wait for this.
        bool loadedJoinData = false;

        /// Connection to the server. Non-blocking after ConnectToServer() succeeds.
        sf::TcpSocket socket;

        /// The other players on the server, not including this client. Parallel to `prevOtherPlayers`
        /// and `prevOtherClocks`: index i in all three refers to the same player.
        std::vector<PlayerData> otherPlayers = {};

        /// Each other player's data from before their most recent update, used to interpolate their
        /// on-screen rectangle. Parallel to `otherPlayers`.
        std::vector<PlayerData> prevOtherPlayers = {};

        /// Time since each other player's data last changed, used as the interpolation progress.
        /// Parallel to `otherPlayers`.
        std::vector<sf::Clock> prevOtherClocks = {};

        /// The planets this client knows about, indexed by planet index. The constructor creates planet 0;
        /// their contents (chunks, entities) are filled in from what the server sends.
        std::vector<std::unique_ptr<Planet>> planets = {};

        /// Chat and server messages shown in the chat window, oldest first.
        std::vector<std::string> chatLog = {};

        /// Packets waiting to be sent to the server, oldest first. FlushOutgoing() sends them and retries
        /// the front packet if it was only partly sent.
        std::deque<sf::Packet> outgoingPackets;

        /// Index into `planets` of the planet the player is on. Set from the server's data when joining.
        int activePlanet = 0;

        /// Measures the time since this client's player data was last sent to the server.
        sf::Clock sendPlayerDataClock;

        /// Seconds between sending this client's player data (camera position, zoom, etc.) to the server.
        /// Also the time over which other players' rectangles are interpolated.
        float timePerPlayerDataUpdate = 0.2f;

        /// @brief Draws the full-screen pause menu: resume, plus "save and quit" for the host or "disconnect" for other players.
        void DisplayPauseMenu();

    public:
        /**
         * @brief Adds a planet to the client.
         * @param planet The planet to add. The client takes ownership and sets the planet's `client` pointer.
         *               Its index is the number of planets before it was added.
         */
        void AddPlanet(Planet *planet);

        /**
         * @brief Per-frame update. Receives and sends network packets, then, once connected and joined,
         *        handles the pause toggle, updates the active planet, requests chunks that are in view
         *        but missing, draws the chat window, and periodically sends this player's data.
         *
         * Also has a debug shortcut: pressing I places a test image made of tiles.
         */
        void DerivedUpdate();

        /// @brief Per-frame drawing: the active planet as seen by its camera, then the other players' view
        ///        rectangles. Draws nothing until connected and join data has loaded.
        void DerivedRender();

        /**
         * @brief Creates the client with one empty planet. Does not connect; see ConnectToServer().
         * @param target The render target to draw to.
         */
        Client(sf::RenderTarget *target);

        /**
         * @brief Connects to a server and sends this player's username.
         *
         * Blocks until the connection succeeds or fails; afterwards the socket is non-blocking.
         * @param ip The server's address.
         * @param port The server's TCP port.
         * @return true if connected (or already connected), false if the connection failed.
         */
        bool ConnectToServer(sf::IpAddress &ip, unsigned short port);

        /**
         * @brief Queues a packet to be sent to the server. It is actually sent by FlushOutgoing().
         * @param packet The packet to send. It is copied. Silently dropped if not connected.
         */
        void SendPacket(sf::Packet &packet);

    private:
        // ---- networking ----

        /**
         * @brief Handles one packet from the server, dispatching on its message type.
         *
         * Handled: JOIN_DATA, PLAYER_JOINED, PLAYER_LEFT, SERVER_MESSAGE, CHAT_MESSAGE, CHUNK_DATA,
         * SET_TILES, ADD_ENTITIES, UPDATE_ENTITIES and UPDATE_PLAYER_DATA. Other types are ignored.
         * @param packet The received packet, positioned at its start.
         */
        void ProcessPacket(sf::Packet &packet);

        /**
         * @brief Handles the server's join data: the list of players (this client's own saved camera is
         *        applied), the chunks around the player, and all entities on the planet.
         * @param packet The JOIN_DATA packet, positioned just after the message type.
         */
        void LoadJoinData(sf::Packet &packet);

        /**
         * @brief Handles another player joining: adds them to the other players and logs it.
         * @param packet The PLAYER_JOINED packet, positioned just after the message type.
         */
        void NewPlayerJoined(sf::Packet &packet);

        /// @brief Adds a player to `otherPlayers`, `prevOtherPlayers` and `prevOtherClocks` together so they stay parallel.
        void AddOtherPlayer(const PlayerData &player);

        /// @brief Removes the first other player with this username from all three parallel lists. Does nothing if not found.
        void RemoveOtherPlayer(const std::string &username);

        /// @brief Whether the connection to the server is open.
        bool IsConnected()
        {
            return connected;
        }

        /// @brief Marks the connection as closed and returns to the main menu. Replaces the current state,
        ///        which destroys this client, so nothing may use it afterwards.
        void OnServerClosed();

        /// @brief Reads and handles every packet currently waiting from the server, and handles disconnection.
        void ReceivePackets();

        /// @brief Sends this player's username, which tells the server to send the join data.
        void SendUsername();

        /// @brief Sends queued packets in order, stopping at the first one that cannot be fully sent
        ///        and retrying it next time. Returns to the main menu if the server disconnected.
        void FlushOutgoing();

    public:
        /// Where a chat log message came from, which decides how it is shown.
        enum class MessageOrigin
        {
            /// A message from this client itself, such as a status update. Shown as "> message".
            SELF,
            /// A message from the server. Shown as "<SERVER> message".
            SERVER,
            /// A chat message from a player. Shown as "<username> message".
            PLAYER
        };

        /// Text typed so far into the chat input box (up to 255 characters).
        char chatInput[256] = "";

        /**
         * @brief Adds a line to the chat log and scrolls to it.
         * @param message The text.
         * @param origin Where it came from, which sets the prefix.
         * @param username The sender's name. Only used when `origin` is PLAYER.
         */
        void LogMessage(std::string message, MessageOrigin origin = MessageOrigin::SELF, std::string username = "");

    private:
        // ---- chat and drawing ----

        /// @brief Logs a chat message locally and sends it to the server.
        void SendChatMessage(const std::string &text);

        /// @brief Draws the chat window: the message log and the input box with a Send button.
        void DrawLogWindow();

        /**
         * @brief Handles a CHUNK_DATA packet by creating the chunks it contains in the active planet.
         *        Chunks that are already loaded are skipped.
         * @param packet The packet, positioned just after the message type.
         */
        void LoadChunks(sf::Packet &packet);

        /// @brief Draws an outline of each other player's visible area on the active planet, in a colour based
        ///        on their username, interpolating between their last two updates.
        void DrawOtherPlayers();

        /// @brief Returns this client's current player data (username, camera position and zoom, planet and
        ///        window size) for sending to the server.
        PlayerData GetPlayerData();


        // --- UNDOCUMENTED STUFF --- 
        std::unique_ptr<KnowledgeGraph> knowledgeGraph;
        bool showingKnowledgeGraph = false;
    };
}