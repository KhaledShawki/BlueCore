// Copyright (c) Khaled Shawki. All rights reserved.

#pragma once

#include <Blue/Memory/Api.h>
#include <Blue/System/Types.h>

namespace Blue
{

enum class MemoryBackendKind
{
  System,
  Mimalloc
};

//////////////////////////////////////////////////////////////////////////////////
// Raw memory backend facade.
//
// This layer normalizes Blue zero-size semantics and dispatches to the selected
// concrete backend at compile time.
//
// Preconditions:
// - alignment must be valid and normalized before reaching this layer.
// - alignment must be a power of two.
// - alignment must be at least alignof(std::max_align_t).
// - Reallocate oldSize must describe the currently allocated block size.
// - Free must receive the same alignment contract used for allocation.
struct BLUE_MEMORY_API MemoryBackend
{
  static void* Allocate( Size size, Size alignment ) noexcept;
  static void* Reallocate( void* pointer, Size oldSize, Size newSize, Size alignment ) noexcept;
  static void Free( void* pointer, Size size, Size alignment ) noexcept;

  static MemoryBackendKind GetKind( ) noexcept;
  static const Char* GetName( ) noexcept;
};
} // namespace Blue
