#pragma once
#include<assert.h>
#include<stdint.h>
#include<stddef.h>
#include<limits.h>
#include<stdbool.h>
#define __STDC_FORMAT_MACROS 1
#include<inttypes.h>

#ifdef __cplusplus
#define C_BEGIN extern "C" {
#define C_END }
#else
#define C_BEGIN
#define C_END
#endif

C_BEGIN

#ifdef DEBUG
#define ASSERT(_EXP) assert(_EXP)
#else
#define ASSERT(_EXP) (_EXP)
#endif

#define STRINGIFY(X) #X
#define TO_STRING(X) STRINGIFY(X)

#define KiB(x) (1024 * (x))
#define MiB(x) (1024 * KiB(x))
#define GiB(x) (1024 * MiB(x))

/* lel */
static_assert(CHAR_BIT == 8, "Char must be 8 bits");

#define BITS_PER_BYTE 8

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef float f32;
typedef double f64;

#define ARRAY_LEN(arr) (sizeof(arr) / sizeof((arr)[0]))

#ifdef __GNUC__

#define CLZ_U64(x) \
    __builtin_clzll((u64)(x))

#endif

#ifdef _WIN32

#define CLZ_U64(x) \
    __lzcnt64((u64)(x))

#endif

// TODO these as functions, instead?

// x + (align - 1) <= ~0, align > 0
#define ALIGN_UP(x, align) \
    ( \
        ( ( ((u64)(x))+((align)-1) ) / (align) ) * (align) \
    )

// x + (align - 1) <= ~0, align > 0, IS_POW_2(align)
#define ALIGN_UP_POW_2(x, align) \
    ( \
        ( ((u64)(x)) + ((align) - 1) ) & (~((align) - 1)) \
    )

// x > 0, x <= 1<<63
#define ROUND_UP_POW_2(x) \
    ( \
        1ULL << (64 - CLZ_U64((x) - 1)) \
    )

// x > 0
#define ROUND_DOWN_POW_2(x) \
    ( \
        1ULL << (63 - CLZ_U64(x)) \
    )

// x > 0
#define IS_POW_2(x) \
    ( \
        ( (x) & ((x) - 1) ) == 0 \
    ) \

// some simple tests
static_assert(CLZ_U64(0) == 64, "CLZ");
static_assert(CLZ_U64(1) == 63, "CLZ");
static_assert(CLZ_U64(1ULL<<62) == 1, "CLZ");
static_assert(CLZ_U64(1ULL<<63) == 0, "CLZ");

static_assert(ALIGN_UP(0, 1) == 0, "ALIGN_UP");
static_assert(ALIGN_UP(1, 1) == 1, "ALIGN_UP");
static_assert(ALIGN_UP(0, 2) == 0, "ALIGN_UP");
static_assert(ALIGN_UP(1, 2) == 2, "ALIGN_UP");
static_assert(ALIGN_UP(2, 2) == 2, "ALIGN_UP");
static_assert(ALIGN_UP(2, 2) == 2, "ALIGN_UP");
static_assert(ALIGN_UP(5, 5) == 5, "ALIGN_UP");
static_assert(ALIGN_UP(6, 10) == 10, "ALIGN_UP");

static_assert(ALIGN_UP_POW_2(1, 1) == 1, "ALIGN_UP_POW_2");
static_assert(ALIGN_UP_POW_2(1, 2) == 2, "ALIGN_UP_POW_2");
static_assert(ALIGN_UP_POW_2(2, 2) == 2, "ALIGN_UP_POW_2");
static_assert(ALIGN_UP_POW_2(3, 2) == 4, "ALIGN_UP_POW_2");
static_assert(ALIGN_UP_POW_2(3, 4) == 4, "ALIGN_UP_POW_2");
static_assert(ALIGN_UP_POW_2(3, 8) == 8, "ALIGN_UP_POW_2");
static_assert(ALIGN_UP_POW_2(38, 32) == 64, "ALIGN_UP_POW_2");
static_assert(ALIGN_UP_POW_2(20, 32) == 32, "ALIGN_UP_POW_2");
static_assert(ALIGN_UP_POW_2(1ULL<<63, 32) == 1ULL<<63, "ALIGN_UP_POW_2");
static_assert(ALIGN_UP_POW_2(32, 1ULL<<63) == 1ULL<<63, "ALIGN_UP_POW_2");

static_assert(ROUND_UP_POW_2(1) == 1, "ROUND_UP_POW_2");
static_assert(ROUND_UP_POW_2(2) == 2, "ROUND_UP_POW_2");
static_assert(ROUND_UP_POW_2(3) == 4, "ROUND_UP_POW_2");
static_assert(ROUND_UP_POW_2(4) == 4, "ROUND_UP_POW_2");
static_assert(ROUND_UP_POW_2(5) == 8, "ROUND_UP_POW_2");
static_assert(ROUND_UP_POW_2(1ULL<<63) == 1ULL<<63, "ROUND_UP_POW_2");

static_assert(ROUND_DOWN_POW_2(1) == 1, "ROUND_DOWN_POW_2");
static_assert(ROUND_DOWN_POW_2(2) == 2, "ROUND_DOWN_POW_2");
static_assert(ROUND_DOWN_POW_2(3) == 2, "ROUND_DOWN_POW_2");
static_assert(ROUND_DOWN_POW_2(4) == 4, "ROUND_DOWN_POW_2");
static_assert(ROUND_DOWN_POW_2(5) == 4, "ROUND_DOWN_POW_2");
static_assert(ROUND_DOWN_POW_2((1ULL<<63) + 1) == 1ULL<<63, "ROUND_DOWN_POW_2");

static_assert(ROUND_UP_POW_2(1) == 1, "ROUND_UP_POW_2");
static_assert(ROUND_UP_POW_2(2) == 2, "ROUND_UP_POW_2");
static_assert(ROUND_UP_POW_2(3) == 4, "ROUND_UP_POW_2");
static_assert(ROUND_UP_POW_2(4) == 4, "ROUND_UP_POW_2");
static_assert(ROUND_UP_POW_2(5) == 8, "ROUND_UP_POW_2");
static_assert(ROUND_UP_POW_2(1ULL<<63) == 1ULL<<63, "ROUND_UP_POW_2");

static_assert(IS_POW_2(1), "IS_POW_2");
static_assert(IS_POW_2(2), "IS_POW_2");
static_assert(!IS_POW_2(3), "IS_POW_2");
static_assert(IS_POW_2(4), "IS_POW_2");
static_assert(!IS_POW_2(5), "IS_POW_2");
static_assert(!IS_POW_2(6), "IS_POW_2");
static_assert(!IS_POW_2(24), "IS_POW_2");
static_assert(IS_POW_2(1ULL<<62), "IS_POW_2");
static_assert(!IS_POW_2((1ULL<<62) + 1), "IS_POW_2");
static_assert(IS_POW_2(1ULL<<63), "IS_POW_2");
static_assert(!IS_POW_2((1ULL<<63) + 1), "IS_POW_2");

C_END
