#pragma once

#ifdef __cplusplus
#define C_HEADER_START extern "C" {
#define C_HEADER_END }
#else
#define C_HEADER_START
#define C_HEADER_END
#endif

C_HEADER_START

#include<assert.h>
#include<stdint.h>
#include<limits.h>
#include<stdbool.h>

#ifdef DEBUG
#define ASSERT(_EXP) assert(_EXP)
#else
#define ASSERT(_EXP) (_EXP)
#endif

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

C_HEADER_END