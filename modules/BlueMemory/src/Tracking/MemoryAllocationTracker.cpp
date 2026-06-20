// Copyright (c) Khaled Shawki. All rights reserved.

#include "Pch.h"

#include <Blue/Memory/Allocation/AllocationValidation.h>
#include <Blue/Memory/Backend/MemoryBackend.h>
#include <Blue/Memory/Tracking/MemoryAllocationTracker.h>
#include <Blue/System/Alignment.h>
#include <Blue/System/Log/LogMacros.h>
#include <Blue/System/Threading/SpinLock.h>

#include <stdio.h>
#include <string.h>

namespace Blue
{
Bool IsMemoryAllocationTrackingCompiledIn( ) noexcept
{
#if BLUE_ENABLE_MEMORY_TRACKING
  return true;
#else
  return false;
#endif
}

#if BLUE_ENABLE_MEMORY_TRACKING
BLUE_DECLARE_LOG_CATEGORY( LogMemory );

namespace
{

constexpr Size DefaultTrackerCapacity = 4096;
constexpr Size MinimumTrackerCapacity = 16;
constexpr Size TrackerLeakLogMessageCapacity = 320;


enum class TrackerSlotState : Uint8
{
  Empty,
  Occupied,
  Tombstone
};

struct MemoryAllocationTrackerSlot
{
  MemoryAllocationRecord Record = { };
  TrackerSlotState State = TrackerSlotState::Empty;
};

struct MemoryAllocationTrackerState
{
  MemoryAllocationTrackerSlot* Slots = nullptr;
  Size Capacity = 0;
  Size ActiveCount = 0;
  Uint64 TotalTrackedCount = 0;
  Uint64 TotalUntrackedCount = 0;
  Uint64 FailedTrackCount = 0;
  Uint64 UnknownFreeCount = 0;
  Uint64 CollisionProbeCount = 0;
  Bool Enabled = false;
  SpinLock Lock = { };
};

constexpr Size trackerAlignment = NormalizeAllocationAlignment( alignof( MemoryAllocationTrackerSlot ) );

MemoryAllocationTrackerState s_Tracker = { };

Size NormalizeTrackerCapacity( Size requestedCapacity ) noexcept
{
  Size capacity = requestedCapacity != 0 ? requestedCapacity : DefaultTrackerCapacity;
  if ( capacity < MinimumTrackerCapacity )
  {
    capacity = MinimumTrackerCapacity;
  }

  if ( IsPowerOfTwo( capacity ) )
  {
    return capacity;
  }

  Size normalized = MinimumTrackerCapacity;
  while ( normalized < capacity && normalized <= ( static_cast< Size >( ~static_cast< Size >( 0 ) ) >> 1 ) )
  {
    normalized <<= 1;
  }

  return normalized;
}

Size HashPointer( void* pointer ) noexcept
{
  NativeUInt value = reinterpret_cast< NativeUInt >( pointer );
  value ^= value >> 33;
  value *= static_cast< NativeUInt >( 0xff51afd7ed558ccdULL );
  value ^= value >> 33;
  value *= static_cast< NativeUInt >( 0xc4ceb9fe1a85ec53ULL );
  value ^= value >> 33;
  return static_cast< Size >( value );
}

Bool IsValidRecord( const MemoryAllocationRecord& record ) noexcept
{
  return record.Pointer != nullptr && record.ByteSize != 0 && record.Alignment != 0 &&
         IsPowerOfTwo( record.Alignment ) && IsValidMemoryPoolId( record.Pool );
}

MemoryAllocationTrackerSlot* FindSlot( void* pointer ) noexcept
{
  if ( !s_Tracker.Enabled || !s_Tracker.Slots || !pointer )
  {
    return nullptr;
  }

  const Size mask = s_Tracker.Capacity - 1;
  Size index = HashPointer( pointer ) & mask;

  for ( Size probe = 0; probe < s_Tracker.Capacity; ++probe )
  {
    MemoryAllocationTrackerSlot& slot = s_Tracker.Slots[ index ];
    if ( slot.State == TrackerSlotState::Empty )
    {
      return nullptr;
    }

    if ( slot.State == TrackerSlotState::Occupied && slot.Record.Pointer == pointer )
    {
      return &slot;
    }

    ++s_Tracker.CollisionProbeCount;
    index = ( index + 1 ) & mask;
  }

  return nullptr;
}

MemoryAllocationTrackerSlot* FindInsertionSlot( void* pointer ) noexcept
{
  const Size mask = s_Tracker.Capacity - 1;
  Size index = HashPointer( pointer ) & mask;
  MemoryAllocationTrackerSlot* firstTombstone = nullptr;

  for ( Size probe = 0; probe < s_Tracker.Capacity; ++probe )
  {
    MemoryAllocationTrackerSlot& slot = s_Tracker.Slots[ index ];
    if ( slot.State == TrackerSlotState::Occupied && slot.Record.Pointer == pointer )
    {
      return &slot;
    }

    if ( slot.State == TrackerSlotState::Tombstone && !firstTombstone )
    {
      firstTombstone = &slot;
    }
    else if ( slot.State == TrackerSlotState::Empty )
    {
      return firstTombstone ? firstTombstone : &slot;
    }

    ++s_Tracker.CollisionProbeCount;
    index = ( index + 1 ) & mask;
  }

  return firstTombstone;
}

void ClearSlot( MemoryAllocationTrackerSlot& slot ) noexcept
{
  slot.Record = { };
  slot.State = TrackerSlotState::Tombstone;
}

const Char* GetSafeText( const Char* text ) noexcept
{
  return text ? text : "<Unknown>";
}

void LogTrackedLeak( const MemoryAllocationRecord& record ) noexcept
{
  Char message[ TrackerLeakLogMessageCapacity ] = { };
  snprintf( message,
            sizeof( message ),
            "Memory live allocation leak detected: pointer=%p size=%llu alignment=%llu pool=%u tag=%u allocator=%u "
            "file=%s function=%s line=%u",
            record.Pointer,
            static_cast< unsigned long long >( record.ByteSize ),
            static_cast< unsigned long long >( record.Alignment ),
            static_cast< Uint32 >( record.Pool ),
            static_cast< Uint32 >( record.Tag ),
            static_cast< Uint32 >( record.Allocator ),
            GetSafeText( record.Location.File ),
            GetSafeText( record.Location.Function ),
            record.Location.Line );

  BLUE_LOG_ERROR( LogMemory, message );
}
} // namespace

Bool InitializeMemoryAllocationTracker( Size capacity ) noexcept
{
  ShutdownMemoryAllocationTracker( );

  const Size normalizedCapacity = NormalizeTrackerCapacity( capacity );
  const Size byteSize = sizeof( MemoryAllocationTrackerSlot ) * normalizedCapacity;

  void* memory = MemoryBackend::Allocate( byteSize, trackerAlignment );
  if ( !memory )
  {
    return false;
  }

  memset( memory, 0, byteSize );
  s_Tracker.Slots = static_cast< MemoryAllocationTrackerSlot* >( memory );
  s_Tracker.Capacity = normalizedCapacity;
  s_Tracker.ActiveCount = 0;
  s_Tracker.TotalTrackedCount = 0;
  s_Tracker.TotalUntrackedCount = 0;
  s_Tracker.FailedTrackCount = 0;
  s_Tracker.UnknownFreeCount = 0;
  s_Tracker.CollisionProbeCount = 0;
  s_Tracker.Enabled = true;
  return true;
}

void ShutdownMemoryAllocationTracker( ) noexcept
{
  ScopedSpinLock lock( s_Tracker.Lock );

  if ( s_Tracker.Slots )
  {
    MemoryBackend::Free( s_Tracker.Slots,
                         sizeof( MemoryAllocationTrackerSlot ) * s_Tracker.Capacity,
                         trackerAlignment );
  }

  s_Tracker.Slots = nullptr;
  s_Tracker.Capacity = 0;
  s_Tracker.ActiveCount = 0;
  s_Tracker.TotalTrackedCount = 0;
  s_Tracker.TotalUntrackedCount = 0;
  s_Tracker.FailedTrackCount = 0;
  s_Tracker.UnknownFreeCount = 0;
  s_Tracker.CollisionProbeCount = 0;
  s_Tracker.Enabled = false;
}

Bool IsMemoryAllocationTrackingEnabled( ) noexcept
{
  return s_Tracker.Enabled;
}

Bool TrackMemoryAllocation( const MemoryAllocationRecord& record ) noexcept
{
  if ( !IsValidRecord( record ) )
  {
    return false;
  }

  ScopedSpinLock lock( s_Tracker.Lock );
  if ( !s_Tracker.Enabled || !s_Tracker.Slots )
  {
    return false;
  }

  MemoryAllocationTrackerSlot* slot = FindInsertionSlot( record.Pointer );
  if ( !slot )
  {
    ++s_Tracker.FailedTrackCount;
    return false;
  }

  if ( slot->State != TrackerSlotState::Occupied )
  {
    ++s_Tracker.ActiveCount;
  }

  slot->Record = record;
  slot->State = TrackerSlotState::Occupied;
  ++s_Tracker.TotalTrackedCount;
  return true;
}

Bool TryFindTrackedMemoryAllocation( void* pointer, MemoryAllocationRecord& outRecord ) noexcept
{
  ScopedSpinLock lock( s_Tracker.Lock );
  MemoryAllocationTrackerSlot* slot = FindSlot( pointer );
  if ( !slot )
  {
    outRecord = { };
    return false;
  }

  outRecord = slot->Record;
  return true;
}

Bool UntrackMemoryAllocation( void* pointer, MemoryAllocationRecord& outRecord ) noexcept
{
  ScopedSpinLock lock( s_Tracker.Lock );
  MemoryAllocationTrackerSlot* slot = FindSlot( pointer );
  if ( !slot )
  {
    outRecord = { };
    if ( s_Tracker.Enabled )
    {
      ++s_Tracker.UnknownFreeCount;
    }
    return false;
  }

  outRecord = slot->Record;
  ClearSlot( *slot );
  --s_Tracker.ActiveCount;
  ++s_Tracker.TotalUntrackedCount;
  return true;
}

Size CaptureLiveMemoryAllocations( MemoryAllocationRecord* outRecords, Size capacity ) noexcept
{
  ScopedSpinLock lock( s_Tracker.Lock );
  if ( !s_Tracker.Enabled || !s_Tracker.Slots || !outRecords || capacity == 0 )
  {
    return 0;
  }

  Size written = 0;
  for ( Size index = 0; index < s_Tracker.Capacity && written < capacity; ++index )
  {
    const MemoryAllocationTrackerSlot& slot = s_Tracker.Slots[ index ];
    if ( slot.State != TrackerSlotState::Occupied )
    {
      continue;
    }

    outRecords[ written++ ] = slot.Record;
  }

  return written;
}

MemoryAllocationTrackerStats GetMemoryAllocationTrackerStats( ) noexcept
{
  ScopedSpinLock lock( s_Tracker.Lock );

  MemoryAllocationTrackerStats stats = { };
  stats.Enabled = s_Tracker.Enabled;
  stats.Capacity = s_Tracker.Capacity;
  stats.ActiveCount = s_Tracker.ActiveCount;
  stats.TotalTrackedCount = s_Tracker.TotalTrackedCount;
  stats.TotalUntrackedCount = s_Tracker.TotalUntrackedCount;
  stats.FailedTrackCount = s_Tracker.FailedTrackCount;
  stats.UnknownFreeCount = s_Tracker.UnknownFreeCount;
  stats.CollisionProbeCount = s_Tracker.CollisionProbeCount;
  return stats;
}

Bool ReportLiveTrackedMemoryAllocations( ) noexcept
{
  Bool leakDetected = false;
  Size nextIndex = 0;

  for ( ;; )
  {
    MemoryAllocationRecord record = { };
    Bool foundRecord = false;

    {
      ScopedSpinLock lock( s_Tracker.Lock );
      if ( !s_Tracker.Enabled || !s_Tracker.Slots )
      {
        return leakDetected;
      }

      for ( Size index = nextIndex; index < s_Tracker.Capacity; ++index )
      {
        const MemoryAllocationTrackerSlot& slot = s_Tracker.Slots[ index ];
        if ( slot.State != TrackerSlotState::Occupied )
        {
          continue;
        }

        record = slot.Record;
        nextIndex = index + 1;
        foundRecord = true;
        break;
      }
    }

    if ( !foundRecord )
    {
      return leakDetected;
    }

    LogTrackedLeak( record );
    leakDetected = true;
  }
}

#else
Bool InitializeMemoryAllocationTracker( Size capacity ) noexcept
{
  static_cast< void >( capacity );
  return false;
}

void ShutdownMemoryAllocationTracker( ) noexcept {}

Bool IsMemoryAllocationTrackingEnabled( ) noexcept
{
  return false;
}

Bool TrackMemoryAllocation( const MemoryAllocationRecord& record ) noexcept
{
  static_cast< void >( record );
  return false;
}

Bool TryFindTrackedMemoryAllocation( void* pointer, MemoryAllocationRecord& outRecord ) noexcept
{
  static_cast< void >( pointer );
  outRecord = { };
  return false;
}

Bool UntrackMemoryAllocation( void* pointer, MemoryAllocationRecord& outRecord ) noexcept
{
  static_cast< void >( pointer );
  outRecord = { };
  return false;
}

Size CaptureLiveMemoryAllocations( MemoryAllocationRecord* outRecords, Size capacity ) noexcept
{
  static_cast< void >( outRecords );
  static_cast< void >( capacity );
  return 0;
}

MemoryAllocationTrackerStats GetMemoryAllocationTrackerStats( ) noexcept
{
  return { };
}

Bool ReportLiveTrackedMemoryAllocations( ) noexcept
{
  return false;
}
#endif
} // namespace Blue
