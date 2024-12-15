load("@rules_cc//cc:defs.bzl", "cc_binary")
load("@rules_platform//platform_data:defs.bzl", "platform_data")
load("@pico-sdk//bazel/util:transition.bzl", "extra_copts_for_all_deps")
load("@pico-sdk//bazel:pico_btstack_make_gatt_header.bzl", "pico_btstack_make_gatt_header")

def _single_example(*, name, example_name, platform_flags = {}, extra_defines = [], extra_deps = [], has_gatt_header):
    base_name = name
    extra_deps = list(extra_deps)
    if has_gatt_header:
        pico_btstack_make_gatt_header(
            name = base_name + "_gatt_header",
            src = "@btstack//:example/{}.gatt".format(example_name),
        )
        extra_deps.append(":{}".format(base_name + "_gatt_header"))

    cc_binary(
        name = base_name,
        deps = extra_deps,
        srcs = ["@btstack//:example/{}.c".format(example_name)],
    )

    extra_copts_for_all_deps(
        name = base_name + "_extra_defines",
        src = ":{}".format(base_name),
        extra_copts = ["-D{}".format(define) for define in extra_defines],
    )

    platform_data(
        name = base_name + "_pico_w.elf",
        platform = ":{}_pico_w_platform".format(base_name),
        target = ":{}_extra_defines".format(base_name),
    )

    platform_data(
        name = base_name + "_pico2_w.elf",
        platform = ":{}_pico2_w_platform".format(base_name),
        target = ":{}_extra_defines".format(base_name),
    )

    native.platform(
        name = base_name + "_pico_w_platform",
        parents = ["//pico_w/bt:test_pico_w"],
        flags = platform_flags,
    )

    native.platform(
        name = base_name + "_pico2_w_platform",
        parents = ["//pico_w/bt:test_pico2_w"],
        flags = platform_flags,
    )

def picow_bt_example(*, extra_defines = [], extra_deps = [], has_gatt_header = False):
    base_name = native.package_name().split("/")[-1]
    _single_example(
        name = base_name + "_poll",
        example_name = base_name,
        extra_deps = extra_deps + [
            "//pico_w/bt:picow_bt_example_poll",
        ],
        has_gatt_header = has_gatt_header,
        platform_flags = [
            "--@pico-sdk//bazel/config:PICO_ASYNC_CONTEXT_IMPL=poll",
        ],
    )

    _single_example(
        name = base_name + "_threadsafe_background",
        example_name = base_name,
        extra_deps = extra_deps + [
            "//pico_w/bt:picow_bt_example_background",
        ],
        has_gatt_header = has_gatt_header,
        platform_flags = [
            "--@pico-sdk//bazel/config:PICO_ASYNC_CONTEXT_IMPL=threadsafe_background",
        ],
    )
