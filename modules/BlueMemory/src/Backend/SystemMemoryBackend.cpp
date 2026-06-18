#include <Blue/Memory/Backend/SystemMemoryBackend.h>

namespace Blue::Backend
{
void* BackendAllocate( Size size, Size alignment );
void* BackendReallocate( void* pointer, Size oldSize, Size newSize, Size alignment );
void BackendFree( void* pointer );
} // namespace Blue::Backend

namespace Blue
{
void* SystemMemoryBackend::Allocate( Size size, Size alignment ) noexcept
{
  return Backend::BackendAllocate( size, alignment );
}

void* SystemMemoryBackend::Reallocate( void* pointer, Size oldSize, Size newSize, Size alignment ) noexcept
{
  return Backend::BackendReallocate( pointer, oldSize, newSize, alignment );
}

void SystemMemoryBackend::Free( void* pointer, Size, Size ) noexcept
{
  Backend::BackendFree( pointer );
}
} // namespace Blue
