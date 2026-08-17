def _string_flag_impl(ctx):
    return []

string_flag = rule(
    implementation = _string_flag_impl,
    build_setting = config.string(flag = True),
)
