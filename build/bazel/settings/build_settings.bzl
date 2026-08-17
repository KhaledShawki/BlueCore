def _string_flag_impl(ctx):
    if ctx.attr.values and ctx.build_setting_value not in ctx.attr.values:
        fail("invalid value '%s'; expected one of %s" % (ctx.build_setting_value, ctx.attr.values))
    return []

string_flag = rule(
    implementation = _string_flag_impl,
    build_setting = config.string(flag = True),
    attrs = {
        "values": attr.string_list(),
    },
)
