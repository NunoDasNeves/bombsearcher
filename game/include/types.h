#pragma once
#include<assert.h>
#include<stdint.h>
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

#define ALIGN_UP(x, align) ((((unsigned long long)(x)) + (align) - 1) / (align))

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

C_END
