#include "timer.hpp"

Timer::Timer(){
    reset();
};

void Timer::reset(){
    start_time = std::chrono::high_resolution_clock::now();
    run_time = 1;
};

void Timer::update(){
    run_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - start_time
    ).count() + 1; // add 1 to avoid divisions by 0
};

int Timer::elapsed() {
    update();
    return run_time;
}