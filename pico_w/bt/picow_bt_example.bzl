load("@rules_cc//cc:defs.bzl", "cc_binary")
load("@rules_platform//platform_data:defs.bzl", "platform_data")
load("@pico-sdk//bazel/util:transition.bzl", "extra_copts_for_all_deps")
load("@pico-sdk//bazel:pico_btstack_make_gatt_header.bzl", "pico_btstack_make_gatt_header")

def _single_example(*, name, extra_defines = [], extra_deps = [], has_gatt_header):
    base_name = name
    extra_deps = list(extra_deps)
    if has_gatt_header:
        pico_btstack_make_gatt_header(
            name = base_name + "_gatt_header",
            src = "@btstack//:example/{}.gatt".format(base_name),
        )
        extra_deps.append(":{}".format(base_name + "_gatt_header"))

    cc_binary(
        name = base_name,
        deps = [
            "//pico_w/bt:picow_bt_example_poll",
        ]+ extra_deps,
        srcs = ["@btstack//:example/{}.c".format(base_name)],
    )

    extra_copts_for_all_deps(
        name = base_name + "_extra_defines",
        src = ":{}".format(base_name),
        extra_copts = ["-D{}".format(define) for define in extra_defines],
    )

    platform_data(
        name = base_name + "_pico_w.elf",
        platform = "//pico_w/bt:test_pico_w",
        target = ":{}_extra_defines".format(base_name),
    )

    platform_data(
        name = base_name + "_pico2_w.elf",
        platform = "//pico_w/bt:test_pico2_w",
        target = ":{}_extra_defines".format(base_name),
    )


def picow_bt_example(*, extra_defines = [], extra_deps = [], has_gatt_header = False):
    base_name = native.package_name().split("/")[-1]
    _single_example(
        name = base_name,
        extra_deps = extra_deps,
        has_gatt_header = has_gatt_header,
        # lwip = True,
        # async_context = "poll",
    )
