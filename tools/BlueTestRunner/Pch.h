#pragma once

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#if defined( _WIN32 )
#	include <Blue/System/Platform/WindowsLean.h>
#else
#	include <sys/wait.h>
#	include <unistd.h>
#endif
