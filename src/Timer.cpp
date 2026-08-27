#include "Timer.hpp"
#include "imgui/imgui.h"
#include <fstream>
#include <iostream>
#include "CC/SaveManager.hpp"
#include "Main.hpp"
namespace Kosmic
{
	namespace Timing
	{
		std::vector<TimeData> data = {};
		int currentIndent;
		sf::Clock timer;
		std::vector<int> stack;
		int numTimes;
		std::string text = "";
		bool displayCollapsed;
		float timePerWrite = 0.5f;
		void Start(std::string name)
		{
			if (data.size() == 0)
			{
				if (name != "Whole Frame")
				{
					return;
				}
				currentIndent = 0;
				numTimes = 0;
				timer.restart();
				stack = {0};
				data.push_back({{}, {}, name, currentIndent});
				data[0].startTimes.push_back(timer.getElapsedTime().asMicroseconds());
			}
			else
			{
				bool existing = false;
				for (int i = 0; i < data.size(); i++)
				{
					if (data[i].name == name)
					{
						if (i == 0)
						{
							numTimes++;
						}
						existing = true;
						stack.push_back(i);
						data[i].startTimes.push_back(timer.getElapsedTime().asMicroseconds());
					}
				}
				if (!existing)
				{
					stack.push_back(data.size());
					data.push_back({{}, {}, name, currentIndent});
					data.back().startTimes.push_back(timer.getElapsedTime().asMicroseconds());
				}
			}
			currentIndent++;
		}
		void End()
		{
			if (data.size() == 0)
			{
				return;
			}
			int index = stack.back();
			stack.pop_back();
			currentIndent--;
			data[index].endTimes.push_back(timer.getElapsedTime().asMicroseconds());
			if (index == 0 && (timer.getElapsedTime().asSeconds() >= timePerWrite && timePerWrite >= 0))
			{
				constexpr bool write = true;
				if (write)
				{
					WriteToGUI();
				}
				data = {};
				currentIndent = 0;
				stack = {};
			}
		}
		void DisplayGUI()
		{
			if (macro.active){
				return;
			}
			Start("imgui");

			ImGui::Begin("Timing", &displayCollapsed, ImGuiWindowFlags_AlwaysAutoResize);
			ImGui::Text("Time Per Write:");
			ImGui::SameLine();
			ImGui::SliderFloat("##", &timePerWrite, -0.01f, 2.f);
			ImGui::Text(text.c_str());
			// ImGui::SetWindowSize(ImGui::getwin)
			ImGui::End();
			End();
		}
		void WriteToFile(std::string path)
		{
			WriteToGUI();
			data = {};
			currentIndent = 0;
			stack = {};
			cc::SaveManager::WriteData(path, text);
		}
		void WriteToGUI()
		{

			std::string result = "";
			double wholeAverage;
			for (int i = 0; i < data.size(); i++)
			{
				for (int j = 0; j < data[i].indentLevel; j++)
				{
					result += " -- ";
				}
				result += data[i].name + ": ";
				uint64_t sum = 0;
				for (int j = 0; j < data[i].endTimes.size(); j++)
				{
					sum += data[i].endTimes[j] - data[i].startTimes[j];
				}
				double avg = (double)(sum) / (1e3 * (double)numTimes);
				if (i == 0)
				{
					wholeAverage = avg;
				}
				result += "avg: " + std::to_string(avg) + "\n";
			}
			text = "FPS: " + std::to_string(1.0 / (wholeAverage / 1000));
			text += '\n';
			text += result;

			// std::ofstream file("timings.txt");
			// file << result;
			// file.close();
			// std::cout << "wrote timings to file" << std::endl;
		}
	}
}