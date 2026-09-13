#include "SaveManager.hpp"
#include "../Main.hpp"
#include "Utils.hpp"
#include <charconv>
#include <sys/stat.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>
#include <chrono>
#include "Item.hpp"
#include "Human.hpp"
namespace cc
{
	namespace SaveManager
	{
		std::string saveName;
		std::string savePath;
		// uint64_t seed;
		sf::Clock playTimeTimer;

		void CreateSave(std::string name, std::string seed)
		{
			playTimeTimer.restart();
			std::string dir = GetSavedataDir();
			if (!DirExists(dir))
			{
				CreateDirectory(dir);
			}
			auto saves = ListDirectories(dir);
			int maxIndex = 0;
			for (auto &s : saves)
			{
				int index = std::stoi(s);
				if (index > maxIndex)
				{
					maxIndex = index;
				}
			}
			maxIndex++;
			saveName = name;
			if (name == "quick start save")
			{
				saveName += " " + std::to_string(maxIndex);
			}
			savePath = dir + "/" + std::to_string(maxIndex);
			CreateDirectory(savePath);
			State *s = new State();
			
			const bool randomize = false;
			if (seed == "" && randomize)
			{
				s->SetSeed(rand());
				// SaveManager::seed = rand();
			}
			else
			{
				s->SetSeed(HashFromString(seed));
				// SaveManager::seed = HashFromString(seed);
			}
			// LoadStartingChunks(s);
			s->renderTarget = window.get();
			s->planets[0]->AddEntity(new Entity());
			Human* h = new Human();		
			h->position = {1.f,0.f};	
			s->planets[0]->AddEntity(h);
			Item* item = new Item();
			item->position = {2.f,0.f};
			s->planets[0]->AddEntity(item);
			InputState inputState;
			s->Update(inputState,0);
			delete state;
			state = s;
			Save(s);
		}
		void Load(int index)
		{
			std::cout << index << std::endl;
			playTimeTimer.restart();
			std::string path = GetSavedataDir();
			auto dirs = ListDirectories(path);
			savePath = path + "/" + dirs[index];

			nlohmann::json j = nlohmann::json::parse(ReadData(savePath + "/metadata.json"));
			saveName = j["saveName"];
			State *s = new State();
			s->SetSeed(j["seed"]);
			InputState inputState;
			s->renderTarget = window.get();
			s->planets[0]->Load();
			s->Update(inputState,0);
			state = s;
		}
		void Save(State *state)
		{
			state->planets[0]->Save();
			WriteMetadata(state);
		}
		void WriteMetadata(State* state)
		{
			struct stat buffer;   
  			bool exists = (stat ((savePath + "/metadata.json").c_str(), &buffer) == 0); 
			nlohmann::json j;
			int playTime = 0;
			if (exists){
				std::string existing = ReadData(savePath + "/metadata.json");
				j = nlohmann::json::parse(existing);
				playTime = j["playTime"];
			}
			playTime += playTimeTimer.restart().asSeconds();
			j["playTime"] = playTime;
			j["saveName"] = saveName;
			auto now = std::chrono::system_clock::now();
			auto duration = now.time_since_epoch();
			auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
			j["modified"] = seconds;
			j["seed"] = state->seed;
			WriteData(savePath + "/metadata.json", j.dump(2));
		}
		void WriteData(std::string path, std::string string)
		{
			std::ofstream file(path);
			auto lines = Split(string, '\n');
			for (int i = 0; i < lines.size(); i++)
			{
				file << lines[i];
				if (i != lines.size() - 1)
				{
					file << std::endl;
				}
			}
			file.close();
		}
		uint64_t HashFromString(std::string &str)
		{
			// Constants for FNV-1a
			const uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
			const uint64_t FNV_PRIME = 1099511628211ULL;

			uint64_t hash = FNV_OFFSET_BASIS;
			for (char c : str)
			{
				hash ^= static_cast<uint64_t>(c);
				hash *= FNV_PRIME;
			}
			return hash;
		}
		bool DirExists(std::string path)
		{
			struct stat info;
			if (stat(path.c_str(), &info) != 0)
				return false;
			return (info.st_mode & S_IFDIR) != 0;
		}
		bool CreateDirectory(std::string path)
		{
#ifdef _WIN32
			return (CreateDirectoryA(path.c_str(), NULL) != 0);
#else
			return (mkdir(path.c_str(), 0755) == 0);
#endif
		}
		std::vector<std::string> ListDirectories(std::string path)
		{
			std::vector<std::string> dirs;
#ifdef _WIN32
			WIN32_FIND_DATAA findFileData;
			HANDLE hfind = FindFirstFileA((path + "/*").c_str(), &findFileData);
			if (hfind == INVALID_HANDLE_VALUE)
			{
				std::cerr << "Failed to list directories in: " << path << std::endl;
				return dirs;
			}
			do
			{
				if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					std::string directoryName = findFileData.cFileName;
					if (directoryName != "." && directoryName != "..")
						dirs.push_back(directoryName);
				}
			} while (FindNextFileA(hfind, &findFileData) != 0);
			FindClose(hfind);
#else
			DIR *dir = opendir(path.c_str());
			if (!dir)
			{
				std::cerr << "Failed to open directory: " << path << std::endl;
				return dirs;
			}
			struct dirent *entry;
			while ((entry = readdir(dir)) != nullptr)
			{
				if (entry->d_type == DT_DIR)
				{
					std::string name = entry->d_name;
					if (name != "." && name != "..")
						dirs.push_back(name);
				}
			}
			closedir(dir);
#endif
			return dirs;
		}

		std::vector<std::string> ListFiles(std::string path)
		{
			std::vector<std::string> files;
#ifdef _WIN32
			WIN32_FIND_DATAA fileData;
			HANDLE hFind = FindFirstFileA((path + "/*").c_str(), &fileData);
			if (hFind != INVALID_HANDLE_VALUE)
			{
				do
				{
					if (!(fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
						files.push_back(path + "/" + fileData.cFileName);
				} while (FindNextFileA(hFind, &fileData) != 0);
				FindClose(hFind);
			}
			else
			{
				std::cerr << "Error opening directory" << std::endl;
			}
#else
			DIR *dir = opendir(path.c_str());
			if (!dir)
			{
				std::cerr << "Failed to open directory: " << path << std::endl;
				return files;
			}
			struct dirent *entry;
			while ((entry = readdir(dir)) != nullptr)
			{
				if (entry->d_type != DT_DIR)
					files.push_back(path + "/" + entry->d_name);
			}
			closedir(dir);
#endif
			return files;
		}

		bool DeleteDirectory(std::string &path)
		{
#ifdef _WIN32
			WIN32_FIND_DATAA findFileData;
			HANDLE hFind = INVALID_HANDLE_VALUE;
			std::string searchPath = path + "/*";
			hFind = FindFirstFileA(searchPath.c_str(), &findFileData);

			if (hFind == INVALID_HANDLE_VALUE)
				return false;

			do
			{
				std::string itemName = findFileData.cFileName;
				if (itemName == "." || itemName == "..")
					continue;

				std::string fullPath = path + "/" + itemName;
				if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					if (!DeleteDirectory(fullPath))
					{
						FindClose(hFind);
						return false;
					}
				}
				else
				{
					if (DeleteFileA(fullPath.c_str()) == 0)
					{
						FindClose(hFind);
						return false;
					}
				}
			} while (FindNextFileA(hFind, &findFileData) != 0);
			FindClose(hFind);
			return (RemoveDirectoryA(path.c_str()) != 0);
#else
			DIR *dir = opendir(path.c_str());
			if (!dir)
				return false;

			struct dirent *entry;
			while ((entry = readdir(dir)) != nullptr)
			{
				std::string name = entry->d_name;
				if (name == "." || name == "..")
					continue;

				std::string fullPath = path + "/" + name;
				struct stat st;
				if (stat(fullPath.c_str(), &st) == 0)
				{
					if (S_ISDIR(st.st_mode))
					{
						if (!DeleteDirectory(fullPath))
						{
							closedir(dir);
							return false;
						}
					}
					else
					{
						if (remove(fullPath.c_str()) != 0)
						{
							closedir(dir);
							return false;
						}
					}
				}
			}
			closedir(dir);
			return (rmdir(path.c_str()) == 0);
#endif
		}

		std::string GetSavedataDir()
		{
#ifdef _WIN32
			const char *userProfile = getenv("USERPROFILE");
			if (!userProfile || !*userProfile)
				throw std::runtime_error("USERPROFILE environment variable not found");
			std::string gamePath = std::string(userProfile) + "/Documents/Games";
			if (!DirExists(gamePath))
			{
				CreateDirectory(gamePath);
			}
			return std::string(userProfile) + "/Documents/Games/Cosmic Construction II";
#else
			const char *home = getenv("HOME");
			if (!home || !*home)
				throw std::runtime_error("HOME environment variable not found");
			std::string gamePath = std::string(home) + "/Documents/Games";
			if (!DirExists(gamePath))
			{
				CreateDirectory(gamePath);
			}
			return std::string(home) + "/Documents/Games/Cosmic Construction II";
#endif
		}
		std::string ReadData(std::string path)
		{
			std::string data = "";
			std::ifstream file(path);
			std::string line;
			while (std::getline(file, line))
			{
				data += line;
				data += '\n';
			}
			// get rid of final new line
			data = data.substr(0, data.size() - 1);
			file.close();
			return data;
		}
	}
	void SaveManager::LoadStartingChunks(State* state){
		int range = 10;
		for (int x = -range; x <= range; x ++){
			for (int z = -range; z <= range; z ++){
				state->planets[0]->chunks[{x, z}] = std::unique_ptr<Chunk>(state->planets[0]->generator.GenerateChunk({x, z}));
			}
		}
	}
}