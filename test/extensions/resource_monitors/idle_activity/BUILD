load(
    "//bazel:envoy_build_system.bzl",
    "envoy_package",
)
load(
    "//test/extensions:extensions_build_system.bzl",
    "envoy_extension_cc_test",
)

licenses(["notice"])  # Apache 2

envoy_package()

envoy_extension_cc_test(
    name = "idle_activity_monitor_test",
    srcs = ["idle_activity_monitor_test.cc"],
    extension_names = ["envoy.resource_monitors.idle_activity"],
    deps = [
        "//source/extensions/resource_monitors/idle_activity:activity_stats_reader",
        "//source/extensions/resource_monitors/idle_activity:idle_activity_monitor",
        "//test/test_common:simulated_time_system_lib",
        "@com_google_absl//absl/types:optional",
        "@envoy_api//envoy/extensions/resource_monitors/idle_activity/v3:pkg_cc_proto",
    ],
)

envoy_extension_cc_test(
    name = "config_test",
    srcs = ["config_test.cc"],
    extension_names = ["envoy.resource_monitors.idle_activity"],
    deps = [
        "//envoy/registry",
        "//source/common/stats:isolated_store_lib",
        "//source/extensions/resource_monitors/idle_activity:config",
        "//source/server:resource_monitor_config_lib",
        "//test/mocks/api:api_mocks",
        "//test/mocks/event:event_mocks",
        "//test/mocks/server:options_mocks",
        "@envoy_api//envoy/extensions/resource_monitors/idle_activity/v3:pkg_cc_proto",
    ],
)
