#pragma once

#include "X_macro_helpers.h"

#define NUM_ARRAY_ELTS(ARRAY)       (sizeof(ARRAY) / sizeof(ARRAY[0]))
#define ARRAY_END(ARRAY)            (ARRAY + NUM_ARRAY_ELTS(ARRAY))

#define EACH_ELT_IN_(ELT_PTR, END_PTR, ARRAY)                  \
    typeof(ARRAY[0]) *ELT_PTR = ARRAY,                         \
                     *END_PTR = ARRAY + NUM_ARRAY_ELTS(ARRAY); \
    ELT_PTR < END_PTR;                                         \
    ++ELT_PTR

#define EACH_ELT_IN(ELT_PTR, ARRAY) EACH_ELT_IN_(ELT_PTR, M_CONCAT(end_, __COUNTER__), ARRAY)

#define EACH_ELT_AFTER_FIRST_IN_(ELT_PTR, END_PTR, ARRAY)      \
    typeof(ARRAY[0]) *ELT_PTR = ARRAY + 1,                     \
                     *END_PTR = ARRAY + NUM_ARRAY_ELTS(ARRAY); \
    ELT_PTR < END_PTR;                                         \
    ++ELT_PTR

#define EACH_ELT_AFTER_FIRST_IN(ELT_PTR, ARRAY) EACH_ELT_AFTER_FIRST_IN_(ELT_PTR, M_CONCAT(end_, __COUNTER__), ARRAY)
