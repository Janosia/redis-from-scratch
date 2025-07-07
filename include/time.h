#pragma once

#include <iostream>
#include <stdint.h>
#include <stddef.h>

const uint64_t k_idle_timeout_ms = 5 * 1000;

static uint64_t get_monotonic_msec();
static int32_t next_timer_ms();