#pragma once
#include "PCH.hpp"

namespace Kosmic
{
	namespace Timing
	{
		struct TimeData
		{
			std::vector<uint64_t> startTimes;
			std::vector<uint64_t> endTimes;
			std::string name;
			int indentLevel;
		};
		extern float timePerWrite;
		extern std::string text;
		extern int currentIndent;
		extern sf::Clock timer;
		extern std::vector<int> stack;
		extern std::vector<TimeData> data;
		extern int numTimes;
		extern bool displayCollapsed;
		void DisplayGUI();
		void Start(std::string name);
		void End();
		void WriteToGUI();
		void WriteToFile(std::string path);
	}
}