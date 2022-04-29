#include <catch2/catch.hpp>
#include <elevated/Types.h>
#include <elevated/algorithm/ProcessAlgorithm.h>

using namespace Elevated;

TEST_CASE("Text based protocol for process algorithm", "[protocol]") {

    BuildingBlueprint building {
        {{0u, 5u, 10u, 15u}, {5u, 15u}},
        {{0}, {1}}
    };

    std::ostringstream str;
    ProcessAlgorithm::write_building(building, str);

    std::string s = str.str();

    REQUIRE(s == "building 2 2\n"
                 "group 0 4 0 5 10 15\n"
                 "group 1 2 5 15\n"
                 "elevator 0 0 1 1 1 1\n"
                 "elevator 1 1 1 1 1 1\n");


}
