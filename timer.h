#ifndef TIMER_H
#define TIMER_H

#include <chrono>

class Timer
{
public:
	Timer();

	void reset() noexcept;
    /* check if not paused before calling tick */
	void tick() noexcept;

	void toggle() noexcept;
	void stop() noexcept;
	void start() noexcept;

    uint16_t getFps() const noexcept;
	float getDeltaTime() const noexcept;
	float getTotalTime() noexcept;
	bool isPaused() const noexcept;

private:
	std::chrono::duration<double> m_deltaTime;
    std::chrono::time_point<std::chrono::steady_clock> m_currTime;
    std::chrono::time_point<std::chrono::steady_clock> m_prevTime;
    std::chrono::time_point<std::chrono::steady_clock> m_baseTime;
	std::chrono::duration<double> m_pausedTime;
    std::chrono::time_point<std::chrono::steady_clock> m_pauseStartTime;
    std::chrono::time_point<std::chrono::steady_clock> m_frameStartTime;

    uint16_t m_fps = 0;
    uint16_t m_frameCount = 0;
	bool m_paused = false;
};

#endif //TIMER_H
