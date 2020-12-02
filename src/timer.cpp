#include "timer.h"

#include <stdint.h>
#include <string.h>

// https://github.com/floooh/sokol/blob/master/sokol_time.h
#define SOKOL_TIME_IMPL
#include <sokol_time.h>

void initTime() { stm_setup(); }
double getTimeInMS() { return stm_ms(stm_now()); }
