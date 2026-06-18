#include <Blue/JobSystem/JobSystem.h>

namespace Blue
{
bool JobSystem::Initialize( Allocator allocator, Uint32 workerCount )
{
  m_Allocator = allocator;
  m_WorkerCount = workerCount;
  m_Initialized = true;
  return true;
}

void JobSystem::Shutdown( )
{
  ExecutePendingJobsOnCurrentThread( );
  m_Initialized = false;
  m_WorkerCount = 0;
  m_Allocator = { };
}

bool JobSystem::Submit( const JobDesc& desc )
{
  if ( !m_Initialized || !desc.Function )
  {
    return false;
  }

  return m_Queue.Push( desc );
}

void JobSystem::ExecutePendingJobsOnCurrentThread( )
{
  JobDesc desc = { };
  while ( m_Queue.Pop( desc ) )
  {
    desc.Function( desc.UserData );
  }
}

bool JobSystem::IsInitialized( ) const
{
  return m_Initialized;
}
} // namespace Blue
