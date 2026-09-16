#pragma once
// #include "State.hpp"
#include "Server.hpp"
namespace cc
{
    namespace SaveManager
    {
        extern std::string saveName;
        extern std::string savePath;
        // extern uint64_t seed;
        void SaveServer(Server* server);
        void CreateSave(std::string name, std::string seed);
        void LoadServer(int index);
        void WriteServerMetadata(Server* server);
        // void LoadStartingChunks(State* state);
        bool CreateDirectory(std::string path);
        bool DirExists(std::string path);
        std::vector<std::string> ListFiles(std::string path);
        std::vector<std::string> ListDirectories(std::string path);
        bool DeleteDirectory(std::string &path);
        uint64_t HashFromString(std::string &str);
        void WriteData(std::string path, std::string string);
        std::string ReadData(std::string path);
        std::string GetSavedataDir();
    }
}