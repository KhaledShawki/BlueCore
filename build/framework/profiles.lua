function bb.apply_profiles()
    filter "configurations:Debug"
        symbols "On"
        optimize "Off"
        runtime "Debug"
        defines {
            "BLUE_DEBUG=1",
            "BLUE_ENABLE_ASSERTS=1",
            "BLUE_ENABLE_LOGGING=1",
            "BLUE_ENABLE_MEMORY_TRACKING=1",
        }

    filter "configurations:Release"
        symbols "On"
        optimize "Speed"
        runtime "Release"
        defines {
            "BLUE_RELEASE=1",
            "NDEBUG",
            "BLUE_ENABLE_ASSERTS=0",
            "BLUE_ENABLE_LOGGING=1",
            "BLUE_ENABLE_MEMORY_TRACKING=0",
        }

    filter "configurations:Profile"
        symbols "On"
        optimize "Speed"
        runtime "Release"
        defines {
            "BLUE_PROFILE=1",
            "NDEBUG",
            "BLUE_ENABLE_ASSERTS=1",
            "BLUE_ENABLE_LOGGING=1",
            "BLUE_ENABLE_MEMORY_TRACKING=1",
            "BLUE_ENABLE_PROFILING=1",
        }

    filter "configurations:Shipping"
        symbols "Off"
        optimize "Full"
        runtime "Release"
        defines {
            "BLUE_SHIPPING=1",
            "NDEBUG",
            "BLUE_ENABLE_ASSERTS=0",
            "BLUE_ENABLE_LOGGING=0",
            "BLUE_ENABLE_MEMORY_TRACKING=0",
        }

    filter {}
end
