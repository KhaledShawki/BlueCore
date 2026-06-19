#pragma once

namespace Blue
{
constexpr Bool IsPowerOfTwo( Size value )
{
  return value != 0 && ( value & ( value - 1u ) ) == 0;
}

constexpr Size AlignUp( Size value, Size alignment )
{
  return IsPowerOfTwo( alignment ) ? ( ( value + alignment - 1u ) & ~( alignment - 1 ) ) : value;
}

constexpr Size AlignDown( Size value, Size alignment )
{
  return IsPowerOfTwo( alignment ) ? ( value & ~( alignment - 1u ) ) : value;
}

constexpr Size AlignPointerUp( NativeUInt value, Size alignment )
{
  return static_cast< Size >( AlignUp( static_cast< Size >( value ), alignment ) );
}

constexpr Size GetAlignmentPadding( NativeUInt value, Size alignment )
{
  const Size aligned = AlignPointerUp( value, alignment );
  return aligned - static_cast< Size >( value );
}
} // namespace Blue
