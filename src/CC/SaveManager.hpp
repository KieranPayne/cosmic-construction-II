#pragma once
#include "Server.hpp"
#include "Serializer.hpp"
namespace cc
{
    /**
     * @brief Creating, loading and saving games on disk, plus the file and folder helpers they use.
     *
     * Saves live in numbered folders inside GetSavedataDir(). The folder of the save currently in use is `savePath`.
     */
    namespace SaveManager
    {
        /// Format used for everything written with WriteSerializerToFile(). JSON files end in ".json", binary ones in ".txt".
        extern Serializer::Format saveFormat;

        /// Name of the save currently in use. Shown in the load menu.
        extern std::string saveName;

        /// Folder of the save currently in use. Set by CreateSave() and LoadServer().
        extern std::string savePath;

        /// The local player's username.
        extern std::string username;

        // ---- saves ----

        /**
         * @brief Saves a server to `savePath`: every planet, the players, and the metadata.
         * @param server The server to save. Players currently connected are stored first so their latest data is included.
         */
        void SaveServer(Server *server);

        /**
         * @brief Makes a new save, starts hosting it, and joins it as a client.
         *
         * Creates the next numbered save folder, makes a server with the given seed, adds a few starting entities,
         * saves it, listens on port 5000, and replaces the current state with a client connected to it.
         * @param name The save's name. The name "quick start save" gets the save's number added to it.
         * @param seed Seed text; hashed with HashFromString() into the world seed.
         */
        void CreateSave(std::string name, std::string seed);

        /**
         * @brief Loads a save, starts hosting it, and joins it as a client.
         * @param index Position of the save's folder in the list from ListDirectories() of GetSavedataDir().
         *              (Not sorted by date.)
         */
        void LoadServer(int index);

        /**
         * @brief Writes the save's metadata.json: name, seed, last modified time, and total play time.
         *
         * Play time is the time already stored in the file plus the time since the last call.
         * @param server The server being saved; its seed is recorded.
         */
        void WriteServerMetadata(Server *server);

        // ---- folders and files ----

        /**
         * @brief Creates a folder. The parent folder must already exist.
         * @param path The folder to create.
         * @return true if it was created.
         */
        bool CreateDirectory(std::string path);

        /**
         * @brief Checks whether a folder exists.
         * @param path The path to check.
         * @return true if the path exists and is a folder.
         */
        bool DirExists(std::string path);

        /**
         * @brief Checks whether a file exists.
         * @param path The path to check.
         * @return true if the path exists and is a regular file.
         */
        bool FileExists(std::string path);

        /**
         * @brief Lists the files directly inside a folder.
         * @param path The folder to list.
         * @return The full path of each file (the folder path, a slash, then the name). Empty if the folder can't be opened.
         */
        std::vector<std::string> ListFiles(std::string path);

        /**
         * @brief Lists the folders directly inside a folder.
         * @param path The folder to list.
         * @return Just the name of each folder (not the full path), without "." and "..". Empty if the folder can't be opened.
         */
        std::vector<std::string> ListDirectories(std::string path);

        /**
         * @brief Deletes a folder and everything inside it.
         * @param path The full path of the folder. A relative path is taken relative to the working directory.
         * @return true if everything was deleted.
         */
        bool DeleteDirectory(std::string &path);

        /**
         * @brief Turns text into a number, using the FNV-1a hash. The same text always gives the same number.
         * @param str The text to hash.
         * @return The 64-bit hash, used for world seeds.
         */
        uint64_t HashFromString(std::string &str);

        /**
         * @brief Gets the folder that holds all saves: "Documents/Games/Cosmic Construction II" in the user's home folder.
         *
         * Creates the "Games" folder if it is missing, but not "Cosmic Construction II" itself.
         * @return The folder path.
         * @throws std::runtime_error if the home folder can't be found.
         */
        std::string GetSavedataDir();

        // ---- reading and writing data ----

        /**
         * @brief Writes text to a file, replacing its contents.
         * @param path The file to write.
         * @param string The text to write. No newline is added at the end.
         */
        void WriteData(std::string path, std::string string);

        /**
         * @brief Reads a text file.
         * @param path The file to read.
         * @return The file's text without its final newline. Empty if the file can't be read.
         */
        std::string ReadData(std::string path);

        /**
         * @brief Writes bytes to a file, replacing its contents. The file starts with the byte count as a uint64_t.
         * @param path The file to write.
         * @param data The bytes to write.
         */
        void WriteBinaryData(std::string path, std::vector<uint8_t> data);

        /**
         * @brief Reads a file written by WriteBinaryData().
         * @param path The file to read.
         * @return The bytes, without the byte count at the start.
         */
        std::vector<uint8_t> ReadBinaryData(std::string path);

        /**
         * @brief Writes a serializer's data to a file, in `saveFormat`.
         * @param s The serializer, in write mode.
         * @param path The file's path without an extension; ".json" or ".txt" is added depending on the format.
         */
        void WriteSerializerToFile(Serializer &s, std::string path);

        /**
         * @brief Reads a file written by WriteSerializerToFile() into a serializer ready for reading.
         * @param path The file's path without an extension, as passed to WriteSerializerToFile().
         * @return A serializer in read mode, in `saveFormat`.
         */
        Serializer LoadSerializerFromFile(std::string path);

        // ---- username ----

        /**
         * @brief Reads the saved username from "username.txt" in the save data folder.
         * @return The username, or an empty string if none has been saved.
         */
        std::string GetUsername();

        /**
         * @brief Saves the username to "username.txt" in the save data folder, creating the folder if needed.
         * @param username The name to save.
         */
        void WriteUsername(std::string username);
    }
}