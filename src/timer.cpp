#include "timer.h"

#include <stdint.h>
#include <string.h>

#if !defined(_WIN32)
#include <unistd.h>
#endif

#include <thread>

// https://github.com/floooh/sokol/blob/master/sokol_time.h
#define SOKOL_TIME_IMPL
#include <sokol/sokol_time.h>

void initTime() { stm_setup(); }
double getTimeInMS() { return stm_ms(stm_now()); }
void sleepInMS(const unsigned int ms)
{
#ifdef _WIN32
    std::this_thread::sleep_for(std::chrono::microseconds(ms));
#else
    ::usleep(ms * 1000);
#endif
}
