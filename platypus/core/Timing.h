#pragma once

#include <chrono>

namespace platypus
{
	class Timing
	{
	private:
		static std::chrono::time_point<std::chrono::high_resolution_clock> s_startFrameTime;
		static float s_deltaTime;

	public:
		static void update();
		static float get_delta_time();
	};
}
