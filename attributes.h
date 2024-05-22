#pragma once

#define ALWAYS_INLINE inline __attribute__((always_inline))
#define PACKED               __attribute__((packed))
#define RAMFUNC              __attribute__((section(".ramfunc,\"awx\",%progbits @")))
#define nodiscard            __attribute__((warn_unused_result))

