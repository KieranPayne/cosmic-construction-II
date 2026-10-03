#pragma once
#include "../PCH.hpp"
#include "Planet.hpp"
#include "PlayerData.hpp"
namespace cc
{
    /**
     * @brief Authoritative game server: owns the world (planets), accepts TCP clients,
     *        applies their requests and relays changes to the other players.
     *
     * Networking model:
     *  - All sockets are non-blocking. Incoming packets are polled in ReceivePackets();
     *    outgoing packets are queued per client and sent by FlushOutgoing().
     *  - A client is connected as soon as it is accepted, but only becomes "joined"
     *    once it has sent its username (see SendJoinData()).
     *  - Clients are identified by a stable uint64_t id, not by their index in `clients`.
     *
     * Threading:
     *  - After StartThread(), the server runs on its own thread and all members belong to
     *    that thread. Any other thread that reads or modifies the server (saving, UI, etc.)
     *    must hold GetMutex() while doing so.
     *  - Before StartThread() (or after Stop()), single-threaded access is safe.
     *  - Members below are public for convenience but are NOT synchronized by themselves.
     */
    class Server
    {
    public:
        /**
         * @brief One connected network client.
         */
        struct ServerClient
        {
            /// Unique id for this connection, assigned by GetNextClientId(). Never reused while the server exists.
            uint64_t id;
            /// The client's TCP connection. Set to non-blocking in AcceptClients().
            sf::TcpSocket socket;
            /// Packets waiting to be sent, oldest first. FlushOutgoing() retries the front packet
            /// until it is fully sent (required because non-blocking sends can be partial).
            std::deque<sf::Packet> outgoing;
            /// This client's player data (camera, planet, username, ...). Only meaningful when `joined` is true.
            PlayerData player;
            /// True once the client has sent its username and been given its join data.
            /// Unjoined clients are ignored by HandlePacket() (except for the username packet).
            bool joined = false;
        };

        // ---- lifecycle ----

        /// @brief Creates the server with a single planet (index 0), flagged as a server planet.
        ///        Does not listen for connections; call Start() for that.
        Server();

        /// @brief Stops the server thread if it is running, so the thread is joined before members are destroyed.
        ~Server();

        /**
         * @brief Begins listening for connections on the given port and makes the listener non-blocking.
         * @param port TCP port to listen on.
         * @note On failure this prints an error and returns without listening. There is no return value,
         *       so callers cannot currently detect failure.
         * @note Does not start the server thread; see StartThread().
         */
        void Start(unsigned short port);

        /**
         * @brief Starts the worker thread that repeatedly calls Update().
         * @return false if the thread was already running, true otherwise.
         * @note Call once the world is fully set up (listening, seed set, planets loaded),
         *       because the thread begins mutating server state immediately.
         */
        bool StartThread();

        /**
         * @brief Signals the worker thread to exit and waits for it to finish.
         *
         * Safe to call repeatedly or when the thread was never started.
         * @warning Do not call while holding GetMutex(): the thread needs the mutex to finish
         *          its current iteration, so this would deadlock.
         */
        void Stop();

        /**
         * @brief Mutex that guards server state while the worker thread is running.
         *
         * The worker holds it for the duration of each Update(). Lock it from other threads
         * before touching the server, for example when saving or reading player lists.
         */
        std::mutex &GetMutex() { return mutex; }

        // ---- simulation ----

        /**
         * @brief Runs one server iteration: accepts new clients, reads incoming packets,
         *        sends queued packets, then runs as many fixed-rate Tick()s as the elapsed time allows.
         * @param dt Seconds since the previous call.
         * @note The tick backlog is capped at 5 ticks, so a long stall does not cause a burst of catch-up ticks.
         * @note Called repeatedly by the server thread; can also be called manually if no thread is used.
         */
        void Update(double dt);

        /// @brief Advances the simulation by one fixed step by ticking every planet (entities, etc.).
        ///        Called at `tps` ticks per second by Update().
        void Tick();

        /**
         * @brief Sets the world seed. Planet i receives the seed `seed + i`.
         * @param seed The base world seed.
         * @note Set this before any chunks are generated.
         */
        void SetSeed(uint64_t seed);

        /// @brief Returns the base world seed passed to SetSeed().
        uint64_t GetSeed();

        // ---- client management ----

        /**
         * @brief Accepts every connection currently waiting on the listener.
         *
         * Each accepted client gets a new id, a non-blocking socket and an empty outgoing queue, and
         * everyone is told "Client connected" via a server log message. The new client is not yet "joined".
         */
        void AcceptClients();

        /// @brief Returns the next unused client id (a counter that only ever increases).
        uint64_t GetNextClientId();

        /**
         * @brief Finds a client by id.
         * @param id Id from ServerClient::id.
         * @return Pointer to the client, or nullptr if no such client exists.
         * @warning The pointer is invalidated when `clients` is modified (a client being accepted or removed),
         *          so do not keep it across calls that can do that.
         */
        ServerClient *GetClient(uint64_t id);

        /**
         * @brief Finds a client's position in `clients`.
         * @param id Id from ServerClient::id.
         * @return Index into `clients`, or -1 if not found.
         * @note Prefer GetClient(); indices shift when other clients are removed.
         */
        int GetIndexOfId(uint64_t id);

        /**
         * @brief Disconnects and removes a client.
         *
         * If the client had joined, its player data is saved into `allPlayers` and a PLAYER_LEFT
         * message is broadcast to the remaining clients. A "Client disconnected" server log is always broadcast.
         * @param index Index into `clients` (not a client id).
         * @warning Invalidates indices and pointers into `clients`.
         */
        void RemoveClient(std::size_t index);

        // ---- player data ----

        /**
         * @brief Stores a player's latest data in `allPlayers`.
         *
         * Overwrites the entry with the same username, or appends a new one if none exists.
         * @param p The player data to store; matched by `p.username`.
         */
        void SavePlayer(const PlayerData &p);

        /**
         * @brief Intended to copy the current data of all connected players into `allPlayers`.
         * @note Declared but not defined in the current Server.cpp. It appears to have been replaced by calling
         *       SavePlayer() for each joined client. Remove the declaration if nothing uses it.
         */
        void RegisterCurrentPlayers();

        // ---- networking: receiving ----

        /**
         * @brief Reads all packets currently waiting from every client and passes each to HandlePacket().
         *
         * Clients whose socket reports a disconnect or error are removed with RemoveClient().
         */
        void ReceivePackets();

        /**
         * @brief Handles one packet from a client, dispatching on its CSMessageType.
         *
         * Handled types: SEND_USERNAME, CHAT_MESSAGE, REQUEST_CHUNKS, REQUEST_SET_TILES,
         * REQUEST_ADD_ENTITIES, REQUEST_UPDATE_ENTITIES and UPDATE_PLAYER_DATA.
         * Tile and entity changes are applied to the sender's planet and relayed to the other joined
         * clients on that planet.
         *
         * Packets from unknown clients are dropped. SEND_USERNAME is only accepted from clients that
         * have not joined yet, and every other type only from clients that have.
         * @param clientId Id of the sending client.
         * @param packet The received packet, positioned at its start; its contents are consumed.
         */
        void HandlePacket(uint64_t clientId, sf::Packet &packet);

        // ---- networking: sending ----

        /**
         * @brief Sends queued packets to each client, in order.
         *
         * Stops for a client at the first packet that could not be fully sent (partial send or socket not ready)
         * and retries that same packet on the next call. Must be called regularly for anything queued by
         * SendToClient() or Broadcast() to actually go out.
         */
        void FlushOutgoing();

        /**
         * @brief Queues a copy of a packet for one client. Nothing is sent until FlushOutgoing().
         * @param clientId Id of the recipient. Does nothing if there is no such client.
         * @param packet Packet to send. It is copied, so the caller's packet can be reused.
         */
        void SendToClient(uint64_t clientId, sf::Packet &packet);

        /**
         * @brief Queues a copy of a packet for every connected client, including ones that have not joined yet.
         * @param packet Packet to send. Each client receives its own copy.
         * @param exclusions Ids of clients to skip (for example, the original sender).
         */
        void Broadcast(sf::Packet &packet, std::vector<uint64_t> exclusions = {});

        /**
         * @brief Completes a client's join: handles its SEND_USERNAME packet.
         *
         * Reads the username, restores that player's saved data from `allPlayers` (or registers a new player),
         * marks the client as joined, and queues a JOIN_DATA packet containing the list of joined players,
         * the chunks around the player's saved camera, and all entities on their planet. Then tells the other
         * clients with PLAYER_JOINED.
         * @param clientId Id of the joining client.
         * @param usernamePacket The SEND_USERNAME packet, positioned just after the message type.
         * @note Generates any visible chunks that do not exist yet, which can be slow for large views.
         */
        void SendJoinData(uint64_t clientId, sf::Packet &usernamePacket);

        /**
         * @brief Queues a CHUNK_DATA packet with the requested chunks from the client's current planet.
         * @param clientId Id of the requesting client.
         * @param positions Chunk coordinates (in chunks, not tiles). Missing chunks are generated on the spot.
         */
        void SendChunks(uint64_t clientId, std::vector<sf::Vector2i> &positions);

        /// @brief Sends a SERVER_MESSAGE (shown in the chat log as coming from the server) to all clients.
        void BroadcastServerLog(std::string message);

        /**
         * @brief Relays a chat message to every client except its sender, tagged with the sender's username.
         * @param message The chat text.
         * @param clientId Id of the client who sent it.
         */
        void BroadcastChatLog(std::string message, uint64_t clientId);

        // ---- state ----

        /// Ticks per second. Used as `1.0 / tps` in Update(), so it must be set to a positive value before the first Update().
        int tps;

        /// All planets in the world. The index in this vector is the planet index used by PlayerData::planet.
        /// The constructor creates planet 0 only.
        std::vector<std::unique_ptr<Planet>> planets = {};

        /// Accepts incoming connections. Non-blocking once Start() succeeds.
        sf::TcpListener listener;

        /// Currently connected clients, in connection order. Indices shift when a client is removed; use ids to refer to clients.
        std::vector<ServerClient> clients;

        /// Id that will be given to the next accepted client. Use GetNextClientId() instead of reading this directly.
        uint64_t currClientId = 0;

        /// Last known data for every player (matched by username) that has joined this save, including
        /// players who are currently offline. Used to restore a returning player's camera and planet.
        std::vector<PlayerData> allPlayers = {};

    private:
        /// Body of the server thread: loops while `running`, waiting briefly for network activity and calling Update() under `mutex`.
        void Run();

        /// Base world seed, as set by SetSeed(). Uninitialized until then.
        uint64_t seed;

        /// Seconds accumulated towards the next tick. Update() adds to it and subtracts one tick interval per Tick().
        /// Has no initial value; it should start at 0.
        double timeSinceTick;

        // keep these in this order: thread must be destroyed before the others
        /// True while the server thread should keep running. Set by StartThread(), cleared by Stop().
        std::atomic<bool> running{false};
        /// Guards server state while the thread is running; see GetMutex().
        std::mutex mutex;
        /// The server thread. Joined by Stop().
        std::thread thread;
    };
}