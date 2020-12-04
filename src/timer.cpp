#include "timer.h"

#include <stdint.h>
#include <string.h>

#if !defined(_WIN32)
#include <unistd.h>
#endif

#include <thread>

#if defined(__clang__)
#pragma clang diagnostic push
#endif
#if defined(_MSC_VER)
#pragma warning(push)
#endif

#if defined(__clang__)
//#pragma clang diagnostic ignored "-Wsign-compare"
#pragma clang diagnostic ignored "-Wsign-conversion"
#endif

#if defined(_MSC_VER)
#pragma warning(disable : 4388)
#endif

// https://github.com/floooh/sokol/blob/master/sokol_time.h
#define SOKOL_TIME_IMPL
#include <sokol/sokol_time.h>

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

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
