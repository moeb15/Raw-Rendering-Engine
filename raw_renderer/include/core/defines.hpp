#pragma once

#include <stdint.h>

#if !defined(_MSC_VER)
    #include <signal.h>
#endif

#define ArraySize(arr) (sizeof(arr)/sizeof(arr[0]))

#if defined(_MSC_VER)
    #define RAW_INLINE                   inline
    #define RAW_FINLINE                  __forceinline
    #define RAW_DEBUG_BREAK              __debugbreak();
    #define RAW_CONCAT_OPERATOR(x, y)    x##y
#else
    #define RAW_INLINE                   inline
    #define RAW_FINLINE                  always_inline
    #define RAW_DEBUG_BREAK              raise(SIGTRAP);
    #define RAW_CONCAT_OPERATOR(x,y)     x y
#endif

#define RAW_STRINGIZE(L)                 #L
#define RAW_MAKESTRING(L)                RAW_STRINGIZE(L)
#define RAW_LINE_STRING                  RAW_MAKESTRING(__LINE__)
#define RAW_FILELINE(msg)                __FILE__ "(" RAW_LINE_STRING ") : " msg
#define RAW_CONCAT(x, y)                 RAW_CONCAT_OPERATOR(x, y)

#ifndef DISABLE_COPY
    #define DISABLE_COPY(className) className(const className&) = delete; className& operator=(const className&) = delete;
#endif

#ifndef DISABLE_MOVE
    #define DISABLE_MOVE(className) className(className&& obj) = delete; className& operator=(className&&) = delete;
#endif

typedef uint8_t                 u8;
typedef uint16_t                u16;
typedef uint32_t                u32;
typedef uint64_t                u64;

typedef int8_t                  i8;
typedef int16_t                 i16;
typedef int32_t                 i32;
typedef int64_t                 i64;

typedef float                   f32;
typedef double                  f64;

typedef const char*             cstring;

static const u64                U64_MAX = UINT64_MAX;
static const i64                I64_MAX = INT64_MAX;
static const u32                U32_MAX = UINT32_MAX;
static const i32                I32_MAX = INT32_MAX;
static const u16                U16_MAX = UINT16_MAX;
static const i16                I16_MAX = INT16_MAX;
static const u8                  U8_MAX = UINT8_MAX;
static const i8                  I8_MAX = INT8_MAX;

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
    #define RAW_PLATFORM_WINDOWS
    #ifndef _WIN64
        #error "64-bit is required on Windows"
    #endif
    #elif defined(__linux__) || defined(__gnu_linux__)
        #define RAW_PLATFORM_LINUX
#endif

#define BIND_EVENT_FN(x) std::bind(&x, this, std::placeholders::_1)

#define RAW_KB(size) (size * 1024)
#define RAW_MB(size) (size * 1024 * 1024)
#define RAW_GB(size) (size * 1024 * 1024 * 1024)