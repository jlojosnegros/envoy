load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_extension",
    "envoy_cc_library",
    "envoy_extension_package",
)

licenses(["notice"])  # Apache 2

envoy_extension_package()

envoy_cc_library(
    name = "activity_stats_reader",
    srcs = ["activity_stats_reader.cc"],
    hdrs = ["activity_stats_reader.h"],
    deps = [
        "//envoy/api:api_interface",
        "//envoy/stats:stats_interface",
        "@com_google_absl//absl/strings",
    ],
)

envoy_cc_library(
    name = "idle_activity_monitor",
    srcs = ["idle_activity_monitor.cc"],
    hdrs = ["idle_activity_monitor.h"],
    deps = [
        ":activity_stats_reader",
        "//envoy/common:time_interface",
        "//envoy/server:resource_monitor_config_interface",
        "//source/common/common:assert_lib",
        "//source/common/protobuf:utility_lib",
        "@com_google_absl//absl/types:optional",
        "@envoy_api//envoy/extensions/resource_monitors/idle_activity/v3:pkg_cc_proto",
    ],
)

envoy_cc_extension(
    name = "config",
    srcs = ["config.cc"],
    hdrs = ["config.h"],
    deps = [
        ":activity_stats_reader",
        ":idle_activity_monitor",
        "//envoy/registry",
        "//source/extensions/resource_monitors/common:factory_base_lib",
        "@envoy_api//envoy/extensions/resource_monitors/idle_activity/v3:pkg_cc_proto",
    ],
)
