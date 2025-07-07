#include <time.h>
#include <stddef.h>
#include <unistd.h>
#include <iostream>

using namespace std;

const uint64_t k_idle_timeout_ms = 5 * 1000;
static uint64_t get_monotonic_msec(){
    struct timespec tv = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &tv);
    return uint64_t(tv.tv_sec) * 1000 + tv.tv_nsec / 1000 / 1000;
}