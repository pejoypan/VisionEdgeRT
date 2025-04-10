#ifndef _TIMER_H_
#define _TIMER_H_

#include <string>
#include <chrono>
#include <iostream>
#include "logging.h"

namespace vert
{
#ifdef VERT_DISABLE_TIMING
class SimpleTimer
{

public:
    SimpleTimer(const std::string &) {}
    ~SimpleTimer() {}

    void start() {}

    void pause() {}

    void resume() {}

    double elapsed() const { return 0.0; }

};

#define TIMEIT(name) void(0);
#define FUNC_TIMER() void(0);

#else // VERT_DISABLE_TIMING
class SimpleTimer
{

public:
    SimpleTimer(const std::string &_name)
        : name_(_name)
    {
        start();
    }
    ~SimpleTimer()
    {
        vert::logger->debug("{} elapsed: {} ms", name_, elapsed());
    }

    void start() {
        start_timepoint_ = std::chrono::high_resolution_clock::now();
        running_ = true;
    }

    void pause() {
       if (running_) {
            auto now = std::chrono::high_resolution_clock::now();
            total_duration_ += now - start_timepoint_;
            running_ = false;
       } 
    }

    void resume() {
        if (!running_) {
            start_timepoint_ = std::chrono::high_resolution_clock::now();
            running_ = true;
        }
    }

    double elapsed() const {
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = now - start_timepoint_;
        return total_duration_.count() + elapsed.count();
    }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_timepoint_;
    std::string name_;
    bool running_ = false;
    std::chrono::duration<double, std::milli> total_duration_;
};

#define TIMEIT(name) vert::SimpleTimer __t(name);
#define FUNC_TIMER() vert::SimpleTimer __t(__func__);

#endif // VERT_DISABLE_TIMING

}


#endif /* _TIMER_H_ */
