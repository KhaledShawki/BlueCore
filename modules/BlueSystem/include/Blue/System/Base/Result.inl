#pragma once

namespace Blue
{
constexpr Bool Result::Succeeded( ) const
{
  return Code == ResultCode::Success;
}

constexpr Bool Result::Failed( ) const
{
  return Code != ResultCode::Success;
}

constexpr Result Success( )
{
  return { ResultCode::Success };
}

constexpr Result Failure( ResultCode code )
{
  return { code };
}
} // namespace Blue
