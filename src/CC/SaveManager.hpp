#pragma once
// #include "State.hpp"
#include "Server.hpp"
#include "Serializer.hpp"
namespace cc
{
    namespace SaveManager
    {
        extern Serializer::Format saveFormat;
        extern std::string saveName;
        extern std::string savePath;
        extern std::string username;
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
        void WriteBinaryData(std::string path, std::vector<uint8_t> data);
        std::vector<uint8_t> ReadBinaryData(std::string path);
        std::string ReadData(std::string path);
        std::string GetSavedataDir();
        //note: do not include file extension, done by function.
        void WriteSerializerToFile(Serializer& s, std::string path);
        Serializer LoadSerializerFromFile(std::string path);
        std::string GetUsername();
        void WriteUsername(std::string username);
        bool FileExists(std::string path);
    }
}