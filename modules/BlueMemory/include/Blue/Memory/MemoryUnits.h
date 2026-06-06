#pragma once

#include <Blue/System/Types.h>

#define BLUE_KB( value ) ( static_cast< ::Blue::Size >( value ) << 10 )
#define BLUE_MB( value ) ( static_cast< ::Blue::Size >( value ) << 20 )
#define BLUE_GB( value ) ( static_cast< ::Blue::Size >( value ) << 30 )
