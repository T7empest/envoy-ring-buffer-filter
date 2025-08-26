load(
    "@envoy//bazel:envoy_build_system.bzl",
    "envoy_cc_binary",
    "envoy_cc_library",
    "envoy_cc_test",
)

package(default_visibility = ["//visibility:public"])

# ---- Envoy static binary that links your HTTP filter factory ----
envoy_cc_binary(
    name = "envoy",
    repository = "@envoy",
    deps = [
        ":link_hcm_always",
        ":link_router_always",
        ":ring_cache_config",
        "@envoy//source/exe:envoy_main_entry_lib",
    ],
)

# --- Force-link Router registration (prevents dead-stripping) ---
envoy_cc_library(
    name = "link_router_always",
    srcs = [],
    repository = "@envoy",
    deps = ["@envoy//source/extensions/filters/http/router:config"],
    alwayslink = 1,
)

# --- Force-link HCM registration (prevents dead-stripping) ---
envoy_cc_library(
    name = "link_hcm_always",
    srcs = [],
    repository = "@envoy",
    deps = ["@envoy//source/extensions/filters/network/http_connection_manager:config"],
    alwayslink = 1,
)

envoy_cc_library(
    name = "ring_cache_core",
    srcs = [
        "ring_buffer.cc",
    ],
    hdrs = [
        "ring_buffer.h",
    ],
    repository = "@envoy",
    deps = [
        "@com_google_absl//absl/hash",
        "@envoy//envoy/common:time_interface",
        "@envoy//envoy/http:filter_interface",
        "@envoy//source/common/common:assert_lib",
        "@envoy//source/common/common:base_logger_lib",
        "@envoy//source/common/common:logger_lib",
        "@envoy//source/common/common:macros",
        "@envoy//source/common/http:header_map_lib",
        "@envoy//source/common/http:utility_lib",
        "@envoy//source/common/protobuf:utility_lib",
    ],
)

# ---- Your HTTP filter implementation (pass-through skeleton) ----
envoy_cc_library(
    name = "ring_cache_lib",
    srcs = ["ring_cache.cc"],
    hdrs = ["ring_cache.h"],
    repository = "@envoy",
    deps = [
        ":ring_cache_core",
        "@envoy//envoy/http:filter_interface",  # HTTP filter API (envoy/http/filter.h)
        "@envoy//source/common/common:logger_lib",
        "@envoy//source/common/http:header_map_lib",  # handy for ResponseHeaderMap helpers
    ],
)

# ---- Factory + registration (REGISTER_FACTORY) ----
envoy_cc_library(
    name = "ring_cache_config",
    srcs = ["ring_cache_config.cc"],  # <-- fixed: was ring_cache.cc
    repository = "@envoy",
    deps = [
        ":ring_cache_lib",
        "@envoy//envoy/registry",
        "@envoy//envoy/server:filter_config_interface",
        "@envoy//source/common/protobuf:utility_lib",  # for ProtobufWkt::Struct, etc.
    ],
)

# ---- Optional: integration test harness (keep if you plan to use it) ----
envoy_cc_test(
    name = "ring_cache_integration_test",
    srcs = ["ring_cache_integration_test.cc"],
    data = ["ring_cache_server.yaml"],
    repository = "@envoy",
    deps = [
        ":ring_cache_config",
        "@envoy//test/integration:http_integration_lib",
        "@envoy//test/test_common:registry_lib",
        "@envoy//test/test_common:utility_lib",
    ],
)

# ---- Optional: sanity binary test ----
sh_test(
    name = "envoy_binary_test",
    srcs = ["envoy_binary_test.sh"],
    data = [":envoy"],
)
