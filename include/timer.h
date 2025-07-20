#pragma once

#include <iostream>
#include <cstdint>
#include <cstddef>

const uint64_t k_idle_timeout_ms = 5 * 1000;

uint64_t get_monotonic_msec();
uint32_t next_timer_ms();