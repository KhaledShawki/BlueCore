#pragma once

#include <stddef.h>
#include <stdint.h>

namespace Blue
{
// =====================================================================
// Primary type definitions
// =====================================================================
using Int8 = int8_t;
using Int16 = int16_t;
using Int32 = int32_t;
using Int64 = int64_t;

using Uint8 = uint8_t;
using Uint16 = uint16_t;
using Uint32 = uint32_t;
using Uint64 = uint64_t;

using Float32 = float;
using Float64 = double;

using Size = size_t;
using PtrDiff = ptrdiff_t;
using NativeUInt = uintptr_t;
using NativeInt = intptr_t;
using Byte = Uint8;

using Bool = bool;
using Bool8 = Uint8;
using Bool32 = Uint32;

using Char = char;
using AnsiChar = char;
using UniChar = char32_t;
using Utf8Char = char8_t;
using Utf16Char = char16_t;
using Utf32Char = char32_t;
using WChar = wchar_t;

// =====================================================================
// Short aliases
// =====================================================================
using u8  = Uint8;
using u16 = Uint16;
using u32 = Uint32;
using u64 = Uint64;

using i8  = Int8;
using i16 = Int16;
using i32 = Int32;
using i64 = Int64;

using f32 = Float32;
using f64 = Float64;

// =====================================================================
// Compile-time size validation
// =====================================================================
static_assert( sizeof( Int8 ) == 1, "Blue::Int8 must be 1 byte." );
static_assert( sizeof( Int16 ) == 2, "Blue::Int16 must be 2 bytes." );
static_assert( sizeof( Int32 ) == 4, "Blue::Int32 must be 4 bytes." );
static_assert( sizeof( Int64 ) == 8, "Blue::Int64 must be 8 bytes." );

static_assert( sizeof( Uint8 ) == 1, "Blue::Uint8 must be 1 byte." );
static_assert( sizeof( Uint16 ) == 2, "Blue::Uint16 must be 2 bytes." );
static_assert( sizeof( Uint32 ) == 4, "Blue::Uint32 must be 4 bytes." );
static_assert( sizeof( Uint64 ) == 8, "Blue::Uint64 must be 8 bytes." );

static_assert( sizeof( Float32 ) == 4, "Blue::Float32 must be 4 bytes." );
static_assert( sizeof( Float64 ) == 8, "Blue::Float64 must be 8 bytes." );

static_assert( sizeof( NativeUInt ) == sizeof( void* ), "Blue::NativeUInt must match pointer size." );
static_assert( sizeof( NativeInt ) == sizeof( void* ), "Blue::NativeInt must match pointer size." );

static_assert( sizeof( Bool ) == 1, "Blue::Bool must be 1 byte on supported platforms." );
static_assert( sizeof( Bool8 ) == 1, "Blue::Bool8 must be 1 byte." );
static_assert( sizeof( Bool32 ) == 4, "Blue::Bool32 must be 4 bytes." );

static_assert( sizeof( Char ) == 1, "Blue::Char must be 1 byte." );
static_assert( sizeof( AnsiChar ) == 1, "Blue::AnsiChar must be 1 byte." );
static_assert( sizeof( Utf8Char ) == 1, "Blue::Utf8Char must be 1 byte." );
static_assert( sizeof( Utf16Char ) == 2, "Blue::Utf16Char must be 2 bytes." );
static_assert( sizeof( Utf32Char ) == 4, "Blue::Utf32Char must be 4 bytes." );
static_assert( sizeof( UniChar ) == 4, "Blue::UniChar must be 4 bytes." );
} // namespace Blue
