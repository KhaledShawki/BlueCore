#include <Blue/System/Result.h>

namespace Blue
{
const Char* GetResultCodeName( ResultCode code )
{
	switch ( code )
	{
		case ResultCode::Success :            return "Success";
		case ResultCode::InvalidArgument :    return "InvalidArgument";
		case ResultCode::OutOfMemory :        return "OutOfMemory";
		case ResultCode::NotInitialized :     return "NotInitialized";
		case ResultCode::AlreadyInitialized : return "AlreadyInitialized";
		case ResultCode::PlatformError :      return "PlatformError";
		case ResultCode::Unsupported :        return "Unsupported";
		case ResultCode::CapacityExceeded :   return "CapacityExceeded";
		case ResultCode::Timeout :            return "Timeout";
		case ResultCode::Busy :               return "Busy";
		case ResultCode::UnknownFailure :     return "UnknownFailure";
		default :                             return "Unknown";
	}
}
} // namespace Blue
