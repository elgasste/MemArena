#if !defined( MA_COMMON_H )
#define MA_COMMON_H

#include <stdint.h>

#define internal static
#define global static
#define local_persist static

#define b32 int32_t
#define True 1
#define False 0
#define TOGGLE_BOOL( b )               b = b ? False : True;

#define UNUSED_PARAM( x )              (void)x

#define STRLEN_MEDIUM                  256

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef float r32;
typedef double r64;

#endif // MA_COMMON_H
