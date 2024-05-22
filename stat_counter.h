#pragma once

typedef uint32_t stat_counter_t;

#define stat_inc(counter_ptr) do {     \
    if (*(counter_ptr) < UINT32_MAX) { \
        ++*(counter_ptr);              \
    }                                  \
} while (false)

#define stat_add(counter_ptr, increment) do {       \
    uint32_t increment___ = (increment);            \
    if (increment___ > UINT32_MAX - *counter_ptr) { \
        *counter_ptr = UINT32_MAX;                  \
    } else {                                        \
        *counter_ptr += increment___;               \
    }                                               \
} while (false)
