#ifndef _TYPES_H_
#define _TYPES_H_

#include<assert.h>
#include<stdint.h>
#include<limits.h>

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

#endif // _TYPES_H_