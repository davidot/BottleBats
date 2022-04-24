#include <catch2/catch.hpp>
#include <elevated/Types.h>
#include <elevated/Building.h>

using namespace Elevated;

TEST_CASE("Building state", "[building][state]") {
    GIVEN("A building") {
        EventListener listener;
        BuildingState building {BuildingBlueprint {
                {{0u, 5u, 10u, 15u}, {5u, 15u}},
                {{0}, {1}}
        }, &listener};

        THEN("All floors have no passengers") {
            REQUIRE(building.passengers_at(0u).empty());
            REQUIRE(building.passengers_at(5u).empty());
            REQUIRE(building.passengers_at(10u).empty());
            REQUIRE(building.passengers_at(15u).empty());
        }

        THEN("The elevators start at its lowest floor") {
            auto& elevator_0 = building.elevator(0);
            auto& elevator_1 = building.elevator(1);
            REQUIRE(elevator_0.group_id == 0); // Technically not required but just assumption for these tests
            REQUIRE(elevator_0.height() == 0);
            REQUIRE(elevator_0.current_state() == Elevated::ElevatorState::State::Stopped);
            REQUIRE(elevator_0.target_height() == elevator_0.height());
            REQUIRE(elevator_0.passengers().empty());
            REQUIRE_FALSE(elevator_0.time_until_next_event().has_value());

            REQUIRE(elevator_1.group_id == 1); // Technically not required but just assumption for these tests
            REQUIRE(elevator_1.height() == 5u);
            REQUIRE(elevator_1.current_state() == Elevated::ElevatorState::State::Stopped);
            REQUIRE(elevator_1.target_height() == elevator_1.height());
            REQUIRE(elevator_1.passengers().empty());
            REQUIRE_FALSE(elevator_1.time_until_next_event().has_value());
        }

        THEN("It does not have a time until next event") {
            REQUIRE_FALSE(building.next_event_at().has_value());
        }

        WHEN("Passenger requests are added") {

            Height first_request_height = GENERATE(0u, 10u, 15u);
            building.add_request({first_request_height, 5, 0});

            THEN("The request is put in only the queue of that floor") {
                REQUIRE(building.passengers_at(5u).empty());

                for (auto floor : {0u, 10u, 15u}) {
                    CAPTURE(floor);
                    REQUIRE(building.passengers_at(floor).size() ==
                            (first_request_height == floor ? 1 : 0));
                }

                auto& queue = building.passengers_at(first_request_height);
                REQUIRE(queue.size() == 1);
                auto& passenger = queue.back();
                REQUIRE(passenger.id != 0);
                REQUIRE(passenger.from == first_request_height);
                REQUIRE(passenger.to == 5u);
                REQUIRE(passenger.group == 0);
            }

            building.add_request({5, 15, 1});

            THEN("The second request ") {
                for (auto floor : {0u, 10u, 15u}) {
                    CAPTURE(floor);
                    REQUIRE(building.passengers_at(floor).size() ==
                            (first_request_height == floor ? 1 : 0));
                }

                REQUIRE(building.passengers_at(5).size() == 1);
                auto& passenger = building.passengers_at(5).back();
                REQUIRE(passenger.id != 0);
                REQUIRE(passenger.from == 5);
                REQUIRE(passenger.to == 15);
                REQUIRE(passenger.group == 1);
                REQUIRE(building.passengers_at(first_request_height).back().id != passenger.id);
            }
        }


        WHEN("Multiple elevator have a set target") {
            auto& elevator0 = building.elevator(0);
            auto& elevator1 = building.elevator(1);
            REQUIRE(elevator0.group_id == 0);
            REQUIRE(elevator1.group_id == 1);

            building.send_elevator(0, 5u);
            building.send_elevator(1, 15u);

            THEN("The elevator have those target and are moving") {
                REQUIRE(elevator0.target_height() == 5u);
                REQUIRE(elevator1.target_height() == 15u);
                REQUIRE(elevator0.current_state() == Elevated::ElevatorState::State::Travelling);
                REQUIRE(elevator1.current_state() == Elevated::ElevatorState::State::Travelling);
                REQUIRE(elevator0.time_until_next_event() < elevator1.time_until_next_event());
            }

            THEN("The buildings next event is the earliest elevator event") {
                auto next_building_event = building.next_event_at();
                REQUIRE(next_building_event.has_value());

                REQUIRE(next_building_event.value() == elevator0.time_until_next_event().value());
                REQUIRE(next_building_event.value() < elevator1.time_until_next_event().value());
            }
        }
    }
}
