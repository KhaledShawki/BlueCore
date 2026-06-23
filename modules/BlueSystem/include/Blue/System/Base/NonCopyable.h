#pragma once

namespace Blue
{
class NonCopyable
{
protected:
  constexpr NonCopyable( ) = default;
  ~NonCopyable( ) = default;

  NonCopyable( NonCopyable&& ) = default;
  NonCopyable& operator=( NonCopyable&& ) = default;

private:
  NonCopyable( const NonCopyable& ) = delete;
  NonCopyable& operator=( const NonCopyable& ) = delete;
};

class NonMovable
{
protected:
  constexpr NonMovable( ) = default;
  ~NonMovable( ) = default;

private:
  NonMovable( const NonMovable& ) = delete;
  NonMovable& operator=( const NonMovable& ) = delete;
  NonMovable( NonMovable&& ) = delete;
  NonMovable& operator=( NonMovable&& ) = delete;
};
} // namespace Blue
