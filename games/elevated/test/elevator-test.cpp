#include <catch2/catch.hpp>
#include <elevated/Types.h>

TEST_CASE("Elevators state", "[elevators]") {
    GIVEN("An elevator starting on 0 height floor") {
        Elevated::Height initial_height = GENERATE(0, 2, 20);
        CAPTURE(initial_height);
        Elevated::ElevatorState elevator{0, 0, initial_height};

        REQUIRE(elevator.current_state() == Elevated::ElevatorState::State::Stopped);

        WHEN("Target set to floor above") {
            Elevated::Height target = GENERATE(1u, 5u, 10u);
            CAPTURE(target);

            elevator.set_target(target);

            THEN("Will set state to the target height") {
                REQUIRE(elevator.height() == initial_height);

                REQUIRE(elevator.target_height() == target);
                REQUIRE(elevator.current_state() == Elevated::ElevatorState::State::Travelling);

                auto next_update_at = elevator.next_event_at();
                REQUIRE(next_update_at.has_value());
                REQUIRE(next_update_at.value() == Elevated::distance_between(initial_height, target) + elevator.door_opening_time);
            }
        }

        WHEN("Target set 0") {
            elevator.set_target(initial_height);

            THEN("Will switch to door opening state") {
                REQUIRE(elevator.height() == initial_height);

                REQUIRE(elevator.target_height() == initial_height);
                REQUIRE(elevator.current_state() == Elevated::ElevatorState::State::DoorsOpening);

                auto next_update_at = elevator.next_event_at();
                REQUIRE(next_update_at.has_value());
                REQUIRE(next_update_at.value() == elevator.door_opening_time);
            }
        }
    }
}
