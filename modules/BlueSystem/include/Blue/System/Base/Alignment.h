#pragma once

#include <Blue/System/Types.h>

namespace Blue
{
constexpr Bool IsPowerOfTwo( Size value );
constexpr Size AlignUp( Size value, Size alignment );
constexpr Size AlignDown( Size value, Size alignment );
constexpr Size AlignPointerUp( NativeUInt value, Size alignment );
constexpr Size GetAlignmentPadding( NativeUInt value, Size alignment );
} // namespace Blue

#include <Blue/System/Base/Alignment.inl>
