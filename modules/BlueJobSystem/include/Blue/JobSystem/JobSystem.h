#pragma once

#include <Blue/Container/RingBuffer.h>
#include <Blue/JobSystem/Api.h>
#include <Blue/Memory/Allocator.h>
#include <Blue/System/Types.h>

namespace Blue
{
using JobFunction = void ( * )( void* userData );

struct JobDesc
{
  JobFunction Function;
  void* UserData;
};

class BLUE_JOB_SYSTEM_API JobSystem
{
public:
  bool Initialize( Allocator allocator, Uint32 workerCount );
  void Shutdown( );

  bool Submit( const JobDesc& desc );
  void ExecutePendingJobsOnCurrentThread( );
  bool IsInitialized( ) const;

private:
  FixedRingBuffer< JobDesc, 4096 > m_Queue;
  Allocator m_Allocator{ };
  Uint32 m_WorkerCount = 0;
  bool m_Initialized = false;
};
} // namespace Blue
