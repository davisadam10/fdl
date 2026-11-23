// SPDX-FileCopyrightText: 2024-present American Society Of Cinematographers
// SPDX-License-Identifier: Apache-2.0
#include <catch2/catch_test_macros.hpp>
#include <jsoncons/json.hpp>

#include "fdl/fdl_core.h"

#include <fstream>
#include <string>

using json = jsoncons::ojson;

static fdl_rounding_mode_t parse_mode(const std::string& s) {
    if (s == "up") {
        return FDL_ROUNDING_MODE_UP;
    }
    if (s == "down") {
        return FDL_ROUNDING_MODE_DOWN;
    }
    return FDL_ROUNDING_MODE_ROUND;
}

static fdl_rounding_even_t parse_even(const std::string& s) {
    if (s == "even") {
        return FDL_ROUNDING_EVEN_EVEN;
    }
    return FDL_ROUNDING_EVEN_WHOLE;
}

TEST_CASE("fdl_round matches Python golden vectors", "[rounding]") {
    std::string path = std::string(VECTORS_DIR) + "/rounding/rounding_vectors.json";
    std::ifstream f(path);
    REQUIRE(f.is_open());

    json data = json::parse(f);
    auto& vectors = data["vectors"];

    for (size_t i = 0; i < vectors.size(); ++i) {
        auto& v = vectors[i];
        double value = v["input"]["value"].as<double>();
        auto mode = parse_mode(v["params"]["mode"].as<std::string>());
        auto even = parse_even(v["params"]["even"].as<std::string>());
        int64_t expected = v["expected"].as<int64_t>();

        CAPTURE(i, value, expected);
        int64_t actual = fdl_round(value, even, mode);
        REQUIRE(actual == expected);
    }
}

TEST_CASE("fdl_round_dimensions rounds both components", "[rounding]") {
    fdl_dimensions_f64_t dims = {19.456, 79.5};
    fdl_dimensions_f64_t result = fdl_round_dimensions(dims, FDL_ROUNDING_EVEN_EVEN, FDL_ROUNDING_MODE_UP);
    REQUIRE(result.width == 20.0);
    REQUIRE(result.height == 80.0);
}

TEST_CASE("fdl_round_point rounds both components", "[rounding]") {
    fdl_point_f64_t point = {19.456, 79.5};
    fdl_point_f64_t result = fdl_round_point(point, FDL_ROUNDING_EVEN_EVEN, FDL_ROUNDING_MODE_UP);
    REQUIRE(result.x == 20.0);
    REQUIRE(result.y == 80.0);
}

TEST_CASE("fdl_round handles NaN and INT64_MAX without UB", "[rounding][ub]") {
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    REQUIRE(fdl_round(nan, FDL_ROUNDING_EVEN_WHOLE, FDL_ROUNDING_MODE_ROUND) == 0);
    REQUIRE(fdl_round(nan, FDL_ROUNDING_EVEN_EVEN, FDL_ROUNDING_MODE_UP) == 0);
    // Clamped to max even int64 without signed overflow
    const int64_t max_even = std::numeric_limits<int64_t>::max() - 1;
    REQUIRE(fdl_round(inf, FDL_ROUNDING_EVEN_EVEN, FDL_ROUNDING_MODE_UP) == max_even);
    REQUIRE(fdl_round(inf, FDL_ROUNDING_EVEN_EVEN, FDL_ROUNDING_MODE_ROUND) == max_even);
    // fp_close(inf, inf) matches Python math.isclose(inf, inf) == True
    REQUIRE(fdl_dimensions_equal({inf, 100.0}, {inf, 100.0}) == FDL_TRUE);
}

TEST_CASE("fdl_geometry_apply_offset honors NULL theoretical pointers", "[geometry][contract]") {
    fdl_geometry_t geo{};
    geo.canvas_dims = {1920.0, 1080.0};
    geo.framing_dims = {1920.0, 800.0};
    geo.framing_anchor = {10.0, 20.0};
    fdl_geometry_t out = fdl_geometry_apply_offset(geo, {5.0, 10.0}, nullptr, nullptr, nullptr);
    REQUIRE(out.framing_anchor.x == 15.0);
    REQUIRE(out.framing_anchor.y == 30.0);
}

TEST_CASE("fdl_doc_set_default_framing_intent clears on NULL", "[builder][contract]") {
    fdl_doc_t* doc = fdl_doc_create_with_header("aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee", 2, 0, "test", "FI_1");
    REQUIRE(doc != nullptr);
    REQUIRE(std::string(fdl_doc_get_default_framing_intent(doc)) == "FI_1");
    fdl_doc_set_default_framing_intent(doc, nullptr);
    REQUIRE(fdl_doc_get_default_framing_intent(doc) == nullptr);
    fdl_doc_free(doc);
}
