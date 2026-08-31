#pragma once

#include <chrono>

class Timer {
    public:
    Timer();
    void reset();
    void update();
    int elapsed();
    private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
    int run_time;
};