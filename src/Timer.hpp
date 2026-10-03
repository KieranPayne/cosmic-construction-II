#pragma once
#include "PCH.hpp"

namespace Kosmic
{
	/**
	 * @brief A simple profiler. Wrap code in Start(name) and End() to see how long it takes.
	 *
	 * Nothing is recorded until a scope called "Whole Frame" is started; that scope is the frame being measured,
	 * and everything started inside it is listed under it, indented by how deeply it is nested. Times are
	 * collected over many frames, then averaged per frame when written.
	 */
	namespace Timing
	{
		/// The recorded times for one named scope.
		struct TimeData
		{
			/// When each run of the scope started, in microseconds since `timer` restarted.
			std::vector<uint64_t> startTimes;

			/// When each run of the scope ended, in microseconds. Matches `startTimes`.
			std::vector<uint64_t> endTimes;

			/// The scope's name.
			std::string name;

			/// How deeply the scope was nested when it first started (0 for "Whole Frame").
			int indentLevel;
		};

		/// Seconds to collect times before writing a result. A negative value turns automatic writing off.
		extern float timePerWrite;

		/// The latest result: the frame rate, then each scope's average time in milliseconds.
		extern std::string text;

		/// How many scopes are open right now.
		extern int currentIndent;

		/// Clock the times are measured with. Restarted when a new batch of times begins.
		extern sf::Clock timer;

		/// Indexes into `data` of the open scopes, innermost last.
		extern std::vector<int> stack;

		/// The scopes recorded so far in this batch. Index 0 is "Whole Frame".
		extern std::vector<TimeData> data;

		/// How many frames are in this batch; the averages are divided by this.
		extern int numTimes;

		/// Passed to ImGui for the "Timing" window's open state.
		extern bool displayCollapsed;

		/// @brief Draws the "Timing" window with the latest result and the write interval slider. Hidden while a macro is running.
		void DisplayGUI();

		/**
		 * @brief Starts timing a scope.
		 * @param name The scope's name. Scopes with the same name are added together. Ignored until "Whole Frame" has started.
		 * @note Every Start() needs a matching End().
		 */
		void Start(std::string name);

		/// @brief Stops timing the most recently started scope. When "Whole Frame" ends and `timePerWrite` has passed, the result is written to `text` and the batch is cleared.
		void End();

		/// @brief Turns the times recorded so far into `text`: the frame rate, then each scope's average time.
		void WriteToGUI();

		/**
		 * @brief Writes the times recorded so far to a file, then clears them.
		 * @param path The file to write.
		 */
		void WriteToFile(std::string path);
	}
}