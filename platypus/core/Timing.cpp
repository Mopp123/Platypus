#include "Timing.h"


namespace platypus
{
	float Timing::s_deltaTime = 0.0f;
	std::chrono::time_point<std::chrono::high_resolution_clock> Timing::s_startFrameTime;

	void Timing::update()
	{
		std::chrono::duration<float> duration = std::chrono::high_resolution_clock::now() - s_startFrameTime;
		s_deltaTime = duration.count();
		s_startFrameTime = std::chrono::high_resolution_clock::now();
	}

	float Timing::get_delta_time()
	{
		return s_deltaTime;
	}
}
