BLUE_COPTS = select({
    "@platforms//os:windows": [
        "/std:c++20",
        "/W4",
        "/permissive-",
        "/EHs-",
        "/EHc-",
        "/GR-",
    ],
    "@platforms//os:linux": [
        "-std=c++20",
        "-Wall",
        "-Wextra",
        "-Wpedantic",
        "-fno-exceptions",
        "-fno-rtti",
    ],
    "@platforms//os:osx": [
        "-std=c++20",
        "-Wall",
        "-Wextra",
        "-Wpedantic",
        "-fno-exceptions",
        "-fno-rtti",
        "-pthread",
    ],
    "//conditions:default": [],
})

BLUE_SHARED_COPTS = select({
    "//build/bazel/settings:linkage_shared_linux": [
        "-fvisibility=hidden",
    ],
    "//build/bazel/settings:linkage_shared_macos": [
        "-fvisibility=hidden",
    ],
    "//conditions:default": [],
})

BLUE_PROFILE_DEFINES = select({
    "//build/bazel/settings:build_profile_debug": [
        "BLUE_DEBUG=1",
        "BLUE_ENABLE_ASSERTS=1",
        "BLUE_ENABLE_LOGGING=1",
        "BLUE_MEMORY_ENABLE_TRACKING=1",
    ],
    "//build/bazel/settings:build_profile_release": [
        "BLUE_RELEASE=1",
        "NDEBUG",
        "BLUE_ENABLE_ASSERTS=0",
        "BLUE_ENABLE_LOGGING=1",
        "BLUE_MEMORY_ENABLE_TRACKING=0",
    ],
    "//build/bazel/settings:build_profile_profile": [
        "BLUE_PROFILE=1",
        "NDEBUG",
        "BLUE_ENABLE_ASSERTS=1",
        "BLUE_ENABLE_LOGGING=1",
        "BLUE_MEMORY_ENABLE_TRACKING=1",
        "BLUE_ENABLE_PROFILING=1",
    ],
    "//build/bazel/settings:build_profile_shipping": [
        "BLUE_SHIPPING=1",
        "NDEBUG",
        "BLUE_ENABLE_ASSERTS=0",
        "BLUE_ENABLE_LOGGING=0",
        "BLUE_MEMORY_ENABLE_TRACKING=0",
    ],
})

BLUE_PLATFORM_DEFINES = select({
    "@platforms//os:windows": [
        "_CRT_SECURE_NO_WARNINGS",
        "NOMINMAX",
        "WIN32_LEAN_AND_MEAN",
    ],
    "//conditions:default": [],
})

BLUE_LINKAGE_DEFINES = select({
    "//build/bazel/settings:linkage_shared": [
        "BLUE_SHARED_LIBRARY=1",
    ],
    "//conditions:default": [],
})

BLUE_TARGET_COMPATIBILITY = select({
    "@platforms//os:windows": [],
    "@platforms//os:linux": [],
    "@platforms//os:osx": [],
    "//conditions:default": ["@platforms//:incompatible"],
})

BLUE_SYSTEM_LINKOPTS = select({
    "@platforms//os:windows": [
        "kernel32.lib",
    ],
    "@platforms//os:linux": [
        "-pthread",
        "-ldl",
        "-lrt",
        "-latomic",
    ],
    "@platforms//os:osx": [
        "-pthread",
    ],
    "//conditions:default": [],
})

BLUE_RUNTIME_LINKOPTS = select({
    "@platforms//os:windows": [
        "kernel32.lib",
    ],
    "@platforms//os:linux": [
        "-pthread",
        "-ldl",
        "-lrt",
    ],
    "@platforms//os:osx": [
        "-pthread",
    ],
    "//conditions:default": [],
})

def blue_shared_consumer_deps(link_requirements):
    return select({
        "//build/bazel/settings:linkage_shared": [native.package_relative_label(link_requirements)],
        "//conditions:default": [],
    })

def blue_shared_dynamic_deps(shared_library):
    return select({
        "//build/bazel/settings:linkage_shared": [native.package_relative_label(shared_library)],
        "//conditions:default": [],
    })

def blue_shared_library_name(name):
    return select({
        "@platforms//os:windows": name + ".dll",
        "@platforms//os:linux": "lib" + name + ".so",
        "@platforms//os:osx": "lib" + name + ".dylib",
        "//conditions:default": "lib" + name + ".so",
    })

def blue_shared_target_compatibility():
    return select({
        "//build/bazel/settings:linkage_shared": [],
        "//conditions:default": ["@platforms//:incompatible"],
    })
