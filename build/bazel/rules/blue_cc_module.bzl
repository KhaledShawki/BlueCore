load("@rules_cc//cc:defs.bzl", "cc_library", "cc_shared_library")
load(
    ":blue_cc_config.bzl",
    "BLUE_COPTS",
    "BLUE_LINKAGE_DEFINES",
    "BLUE_PLATFORM_DEFINES",
    "BLUE_PROFILE_DEFINES",
    "BLUE_SHARED_COPTS",
    "BLUE_TARGET_COMPATIBILITY",
    "blue_shared_library_name",
    "blue_shared_target_compatibility",
)

def _blue_cc_module_impl(
        name,
        visibility,
        configured_defines,
        defines,
        deps,
        hdrs,
        link_requirement_deps,
        linkopts,
        local_defines,
        local_includes,
        shared_dynamic_deps,
        shared_local_defines,
        srcs):
    cc_library(
        name = name + "_link_requirements",
        deps = link_requirement_deps,
        linkopts = linkopts,
        linkstatic = True,
        target_compatible_with = BLUE_TARGET_COMPATIBILITY,
        visibility = visibility,
    )

    cc_library(
        name = name,
        srcs = srcs,
        hdrs = hdrs,
        copts = BLUE_COPTS + BLUE_SHARED_COPTS,
        defines = defines + configured_defines + BLUE_PROFILE_DEFINES + BLUE_PLATFORM_DEFINES + BLUE_LINKAGE_DEFINES,
        local_defines = local_defines + select({
            "//build/bazel/settings:linkage_shared": shared_local_defines,
            "//conditions:default": [],
        }),
        deps = deps,
        linkopts = linkopts,
        linkstatic = True,
        local_includes = local_includes,
        strip_include_prefix = "include",
        target_compatible_with = BLUE_TARGET_COMPATIBILITY,
        visibility = visibility,
    )

    cc_shared_library(
        name = name + "_shared",
        deps = [native.package_relative_label(":" + name)],
        dynamic_deps = shared_dynamic_deps,
        shared_lib_name = blue_shared_library_name(name),
        target_compatible_with = blue_shared_target_compatibility(),
        visibility = visibility,
    )

    native.alias(
        name = name + "_artifact",
        actual = select({
            "//build/bazel/settings:linkage_static": native.package_relative_label(":" + name),
            "//build/bazel/settings:linkage_shared": native.package_relative_label(":" + name + "_shared"),
        }),
        visibility = visibility,
    )

blue_cc_module = macro(
    attrs = {
        "configured_defines": attr.string_list(default = []),
        "defines": attr.string_list(default = [], configurable = False),
        "deps": attr.label_list(default = []),
        "hdrs": attr.label_list(default = [], allow_files = True),
        "link_requirement_deps": attr.label_list(default = [], configurable = False),
        "linkopts": attr.string_list(default = []),
        "local_defines": attr.string_list(default = [], configurable = False),
        "local_includes": attr.string_list(default = [], configurable = False),
        "shared_dynamic_deps": attr.label_list(default = [], configurable = False),
        "shared_local_defines": attr.string_list(default = [], configurable = False),
        "srcs": attr.label_list(default = [], allow_files = True),
    },
    implementation = _blue_cc_module_impl,
)
