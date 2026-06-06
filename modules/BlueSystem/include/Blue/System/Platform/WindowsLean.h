#pragma once

#if !defined( _WIN32 )
#	error "Blue/System/Platform/WindowsLean.h can only be included by Windows platform code."
#endif

#ifndef WIN32_LEAN_AND_MEAN
#	define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#	define NOMINMAX
#endif

#include <windows.h>
