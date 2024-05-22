#pragma once

#include "attributes.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

// This ring buffer requires the underlying buffer size to be a power of 2, and
// uses unsigned indexes into the buffer in such a way that they can be
// subtracted to count the number of bytes, and compared for equality to
// determine emptiness.

typedef struct {
             uint8_t *buf;
             size_t   buf_size_bytes;
    volatile size_t   head;
    volatile size_t   tail;
} ring_buffer_t;

#define RB_CREATE(uint8_t_buffer_array) ({                                                                                                   \
    _Static_assert(((sizeof(uint8_t_buffer_array) - 1) & sizeof(uint8_t_buffer_array)) == 0, "ring_buffer.size_bytes must be a power of 2"); \
                                                                                                                                             \
    ring_buffer_t rb = {                                                                                                                     \
        .buf            = (uint8_t *)(void *)&uint8_t_buffer_array,                                                                          \
        .buf_size_bytes = sizeof(uint8_t_buffer_array),                                                                                      \
    };                                                                                                                                       \
                                                                                                                                             \
    rb;                                                                                                                                      \
})

static RAMFUNC ALWAYS_INLINE nodiscard bool rb_is_empty(const ring_buffer_t *rb) {
    return rb->tail == rb->head;
}

static RAMFUNC ALWAYS_INLINE nodiscard size_t rb_num_bytes(const ring_buffer_t *rb) {
    return rb->head - rb->tail; // unsigned overflow expected when head wraps to be numerically lower than tail.
}

static RAMFUNC ALWAYS_INLINE nodiscard bool rb_is_full(const ring_buffer_t *rb) {
    return rb_num_bytes(rb) >= rb->buf_size_bytes;
}

static RAMFUNC ALWAYS_INLINE nodiscard size_t rb_num_free_bytes(const ring_buffer_t *rb) {
    return rb->buf_size_bytes - rb_num_bytes(rb);
}

static inline RAMFUNC bool nodiscard rb_pop(ring_buffer_t *rb, uint8_t *data) {
    if (!rb_is_empty(rb)) {
        *data = rb->buf[rb->tail % rb->buf_size_bytes];
        ++rb->tail; // Do this last for interrupt safety
        return true;
    }

    return false;
}

static inline RAMFUNC void rb_push_assuming_room(ring_buffer_t *rb, uint8_t b) {
    rb->buf[rb->head % rb->buf_size_bytes] = b;
    ++rb->head; // Do this last for interrupt safety
}

static inline RAMFUNC nodiscard bool rb_push(ring_buffer_t *rb, uint8_t b) {
    if (!rb_is_full(rb)) {
        rb_push_assuming_room(rb, b);
        return true;
    }

    return false;
}

