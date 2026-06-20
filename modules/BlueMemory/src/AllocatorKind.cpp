// Copyright (c) Khaled Shawki. All rights reserved.

#include "Pch.h"

#include <Blue/Memory/AllocatorKind.h>


namespace Blue
{
const Char* GetAllocatorKindName( AllocatorKind kind ) noexcept
{
  switch ( kind )
  {
    case AllocatorKind::Default :   return "Default";
    case AllocatorKind::Heap :      return "Heap";
    case AllocatorKind::Linear :    return "Linear";
    case AllocatorKind::Stack :     return "Stack";
    case AllocatorKind::FixedPool : return "FixedPool";
    case AllocatorKind::Slot :      return "Slot";
    case AllocatorKind::Tlsf :      return "Tlsf";
    case AllocatorKind::BigBlock :  return "BigBlock";
    case AllocatorKind::Frame :     return "Frame";
    case AllocatorKind::Count :
    default :                       return "Unknown";
  }
}
} // namespace Blue
