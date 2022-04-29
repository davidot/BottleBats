#include <catch2/catch.hpp>
#include <elevated/Types.h>
#include <elevated/algorithm/ProcessAlgorithm.h>

using namespace Elevated;

TEST_CASE("Info levels for process algorithm", "[protocol]") {
    GIVEN("A building and algorithm at level=Low") {
        ProcessAlgorithm algorithm { { "!do not run!" }, Elevated::ProcessAlgorithm::InfoLevel::Low };
        EventListener listener;
        BuildingState building {BuildingBlueprint {
             {{0u, 5u, 10u, 15u}, {5u, 15u}},
             {{0}, {1}}
        }, &listener};

        WHEN("There is a single request on a floor") {
            Height floor = GENERATE(0u, 5u);
            Height to_floor = GENERATE(10u, 15u);
            auto index = building.add_request({floor, to_floor, 0, 0});
            REQUIRE(index.has_value());
            auto result = algorithm.should_write_new_request(building, floor, index.value());

            THEN("It should write the request") {
                REQUIRE(result);
            }
        }

        WHEN("There already is a identical request on the floor") {
            Height floor = GENERATE(0u, 5u);
            Height to_floor = GENERATE(10u, 15u);
            REQUIRE(building.add_request({floor, to_floor, 0, 0}).has_value());
            auto index = building.add_request({floor, to_floor, 0, 0});
            REQUIRE(index.has_value());
            auto result = algorithm.should_write_new_request(building, floor, index.value());

            THEN("It should not write the request") {
                REQUIRE_FALSE(result);
            }
        }

        WHEN("There already is a request on the floor with a different group") {
            Height floor = 5u;
            Height to_floor = 15u;
            REQUIRE(building.add_request({floor, to_floor, 1, 0}).has_value());
            auto index = building.add_request({floor, to_floor, 0, 0});
            REQUIRE(index.has_value());
            auto result = algorithm.should_write_new_request(building, floor, index.value());

            THEN("It should write the request") {
                REQUIRE(result);
            }
        }

        WHEN("There already is a request on the floor with the same direction") {
            Height floor = 5u;
            REQUIRE(building.add_request({floor, 10u, 0, 0}).has_value());
            auto index = building.add_request({floor, 15u, 0, 0});
            REQUIRE(index.has_value());
            auto result = algorithm.should_write_new_request(building, floor, index.value());

            THEN("It should not write the request") {
                REQUIRE_FALSE(result);
            }
        }

        WHEN("There already is a request on the floor with the opposite direction") {
            Height floor = 5u;
            REQUIRE(building.add_request({floor, 0u, 0, 0}).has_value());
            auto index = building.add_request({floor, 15u, 0, 0});
            REQUIRE(index.has_value());
            auto result = algorithm.should_write_new_request(building, floor, index.value());

            THEN("It should write the request") {
                REQUIRE(result);
            }
        }
    }
}
