#include <catch2/catch.hpp>
#include <elevated/Types.h>

TEST_CASE("Elevators state", "[elevators]") {
    GIVEN("An initialized elevator") {
        Elevated::ElevatorID id = GENERATE(0, 2, 20);
        Elevated::GroupID group_id = GENERATE(0, 2, 20);
        Elevated::Height initial_height = GENERATE(0, 2, 20);
        CAPTURE(id, group_id, initial_height);
        Elevated::ElevatorState elevator{id, group_id, initial_height};

        THEN("It has no passengers") {
            REQUIRE(elevator.passengers().empty());
        }

        THEN("It starts at the given height and target") {
            REQUIRE(elevator.height() == initial_height);
            REQUIRE(elevator.target_height() == initial_height);
        }

        THEN("It is stopped") {
            REQUIRE(elevator.current_state() == Elevated::ElevatorState::State::Stopped);
        }
    }


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

                auto next_update_at = elevator.time_until_next_event();
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

                auto next_update_at = elevator.time_until_next_event();
                REQUIRE(next_update_at.has_value());
                REQUIRE(next_update_at.value() == elevator.door_opening_time);
            }
        }
    }

    GIVEN("An elevator travelling to another floor") {
        Elevated::Height initial_height = 0;
        Elevated::ElevatorState elevator{0, 0, initial_height};

        Elevated::Height target_height = 10;
        elevator.set_target(target_height);

        REQUIRE(elevator.current_state() == Elevated::ElevatorState::State::Travelling);

        auto steps = elevator.time_until_next_event();
        REQUIRE(steps.has_value());

        WHEN("Elevator is updated the amount until next changed") {
            auto result = elevator.update(steps.value());

            THEN("Is at the correct height and has doors opened result") {
                REQUIRE(result == Elevated::ElevatorState::ElevatorUpdateResult::DoorsOpened);
                REQUIRE(elevator.height() == target_height);
                REQUIRE_FALSE(elevator.time_until_next_event().has_value());
            }
        }

        WHEN("Elevator is updated less than amount needed for update") {
            auto result = elevator.update(steps.value() - 1);

            THEN("No change is emitted") {
                REQUIRE(result == Elevated::ElevatorState::ElevatorUpdateResult::Nothing);
                REQUIRE((elevator.current_state() == Elevated::ElevatorState::State::Travelling
                         || elevator.current_state() == Elevated::ElevatorState::State::DoorsOpening));
                REQUIRE(elevator.time_until_next_event() == 1);
            }
        }

        WHEN("Elevator is updated not enough to travel") {
            REQUIRE(steps > 5);
            auto result = elevator.update(5);

            THEN("No change is emitted and elevator is still travelling") {
                REQUIRE(result == Elevated::ElevatorState::ElevatorUpdateResult::Nothing);
                REQUIRE(elevator.current_state() == Elevated::ElevatorState::State::Travelling);
                REQUIRE(elevator.height() == initial_height + 5);
                REQUIRE(elevator.time_until_next_event() == steps.value() - 5);
            }
        }

        WHEN("Elevator is update just enough to travel") {
            REQUIRE(steps > 10);

            auto result = elevator.update(10);

            THEN("No change is emitted but state is in door opening") {
                REQUIRE(result == Elevated::ElevatorState::ElevatorUpdateResult::Nothing);
                REQUIRE(elevator.current_state() == Elevated::ElevatorState::State::DoorsOpening);
                REQUIRE(elevator.height() == target_height);
                REQUIRE(elevator.time_until_next_event() == steps.value() - 10);
            }
        }

        WHEN("Elevator is updated in steps") {
            auto steps_to_go = steps.value();

            while (steps_to_go > 1) {
                --steps_to_go;
                auto result = elevator.update(1);
                THEN("Nothing happens for all but the last step") {
                    REQUIRE(result == Elevated::ElevatorState::ElevatorUpdateResult::Nothing);
                    REQUIRE((elevator.current_state() == Elevated::ElevatorState::State::Travelling
                             || elevator.current_state() ==
                                Elevated::ElevatorState::State::DoorsOpening));
                    REQUIRE(elevator.time_until_next_event() == steps_to_go);
                }
            }

            REQUIRE(steps_to_go == 1);
            auto final_result = elevator.update(1);

            THEN("And for the last step it does give a change and doors are opened") {
                REQUIRE(final_result == Elevated::ElevatorState::ElevatorUpdateResult::DoorsOpened);
                REQUIRE(elevator.height() == target_height);
                REQUIRE(elevator.current_state() == Elevated::ElevatorState::State::DoorsOpen);
                REQUIRE_FALSE(elevator.time_until_next_event().has_value());
            }
        }
    }

    GIVEN("An elevator waiting with dooropened") {
        Elevated::GroupID group_id = GENERATE(0, 2, 3);
        Elevated::GroupID other_group_id = GENERATE(1, 4);
        CAPTURE(group_id, other_group_id);

        Elevated::ElevatorState elevator {0, group_id, 0};
        elevator.set_target(0);

        REQUIRE(elevator.current_state() == Elevated::ElevatorState::State::DoorsOpening);
        auto steps = elevator.time_until_next_event();
        REQUIRE(steps.has_value());
        auto result = elevator.update(steps.value());
        REQUIRE(result == Elevated::ElevatorState::ElevatorUpdateResult::DoorsOpened);
        REQUIRE(elevator.current_state() == Elevated::ElevatorState::State::DoorsOpen);
        REQUIRE_FALSE(elevator.time_until_next_event().has_value());

        WHEN("Picking up from empty floor") {
            std::vector<Elevated::Passenger> line;
            elevator.transfer_passengers(line);

            THEN("No passengers were picked up and doors are closing") {
                REQUIRE(line.empty());
                REQUIRE(elevator.passengers().empty());
                REQUIRE(elevator.current_state() == Elevated::ElevatorState::State::DoorsClosing);
                REQUIRE(elevator.time_until_next_event() == elevator.door_closing_time);
            }
        }

        WHEN("Picking up from line with no passengers of matching group id") {
            std::vector<Elevated::Passenger> line;
            auto count = GENERATE(1, 2, 10);
            CAPTURE(count);
            for (auto i = 0u; i < count; ++i)
                line.push_back(Elevated::Passenger{i, 0, 1, other_group_id});

            REQUIRE(line.size() == count);

            elevator.transfer_passengers(line);

            THEN("No passengers were picked up and doors are closing") {
                CAPTURE(line.size(), elevator.passengers().size());
                REQUIRE(line.size() == count);
                REQUIRE(elevator.passengers().empty());
                REQUIRE(elevator.current_state() == Elevated::ElevatorState::State::DoorsClosing);
                REQUIRE(elevator.time_until_next_event() == elevator.door_closing_time);

                REQUIRE(std::is_sorted(line.begin(), line.end(), [&](Elevated::Passenger const& lhs, Elevated::Passenger const& rhs) {
                    return lhs.id < rhs.id;
                }));
            }
        }

        WHEN("Picking up from line with only passengers of matching group id") {
            std::vector<Elevated::Passenger> line;
            auto count = GENERATE(1, 2, 10);
            CAPTURE(count);
            for (auto i = 0u; i < count; ++i)
                line.push_back(Elevated::Passenger{i, 0, 1, group_id});

            elevator.transfer_passengers(line);

            THEN("Picks up all passengers and doors are closing") {
                REQUIRE(line.empty());
                REQUIRE(elevator.passengers().size() == count);

                for (auto i = 0; i < count; ++i) {
                    CAPTURE(i);
                    REQUIRE(std::find_if(elevator.passengers().begin(), elevator.passengers().end(),
                                 [&](Elevated::ElevatorState::TravellingPassenger const &p) {
                                     return p.id == i;
                                 }) != elevator.passengers().end());
                }

                REQUIRE(elevator.current_state() == Elevated::ElevatorState::State::DoorsClosing);
                REQUIRE(elevator.time_until_next_event() == elevator.door_closing_time);
            }
        }

        WHEN("Picking up from a mixed line of waiting passengers") {
            std::vector<Elevated::Passenger> line {
                {0, 0, 1, group_id},
                {1, 0, 1, other_group_id},
                {2, 0, 1, other_group_id},
                {3, 0, 1, group_id},
                {4, 0, 1, group_id},
                {5, 0, 1, other_group_id},
                {6, 0, 1, other_group_id},
            };

            elevator.transfer_passengers(line);

            THEN("Picks up all passengers and doors are closing") {
                REQUIRE(line.size() == 4);
                REQUIRE(elevator.passengers().size() == 3);

                for (auto i : {0, 3, 4}) {
                    CAPTURE(i);
                    REQUIRE(std::find_if(elevator.passengers().begin(), elevator.passengers().end(),
                                         [&](Elevated::ElevatorState::TravellingPassenger const &p) {
                                             return p.id == i;
                                         }) != elevator.passengers().end());
                }

                REQUIRE(elevator.current_state() == Elevated::ElevatorState::State::DoorsClosing);
                REQUIRE(elevator.time_until_next_event() == elevator.door_closing_time);

                REQUIRE(std::is_sorted(line.begin(), line.end(), [&](Elevated::Passenger const& lhs, Elevated::Passenger const& rhs) {
                    return lhs.id < rhs.id;
                }));
            }

        }
    }

    GIVEN("An elevator which has just picked up passengers and is now closing doors") {
        Elevated::ElevatorState elevator {0, 0, 0};
        elevator.set_target(0);

        {
            REQUIRE(elevator.current_state() == Elevated::ElevatorState::State::DoorsOpening);
            auto steps = elevator.time_until_next_event();
            REQUIRE(steps.has_value());
            auto result = elevator.update(steps.value());
            REQUIRE(result == Elevated::ElevatorState::ElevatorUpdateResult::DoorsOpened);
            REQUIRE(elevator.current_state() == Elevated::ElevatorState::State::DoorsOpen);
            REQUIRE_FALSE(elevator.time_until_next_event().has_value());
            std::vector<Elevated::Passenger> line;
            elevator.transfer_passengers(line);
            REQUIRE(elevator.current_state() == Elevated::ElevatorState::State::DoorsClosing);
            REQUIRE(elevator.passengers().empty());
        }

        WHEN("Elevator is updated until next event") {
            auto steps = elevator.time_until_next_event();
            REQUIRE(steps.has_value());
            auto result = elevator.update(steps.value());

            THEN("Gives doors closed result and is stopped") {
                REQUIRE(result == Elevated::ElevatorState::ElevatorUpdateResult::DoorsClosed);
                REQUIRE(elevator.current_state() == Elevated::ElevatorState::State::Stopped);
                REQUIRE_FALSE(elevator.time_until_next_event().has_value());
            }
        }

        WHEN("Target of same floor is set before doors are closed") {
            elevator.set_target(0);

            auto steps = elevator.time_until_next_event();
            REQUIRE(steps.has_value());
            auto result = elevator.update(steps.value());

            THEN("Does not target floor and stays stopped") {
                REQUIRE(result == Elevated::ElevatorState::ElevatorUpdateResult::DoorsClosed);
                REQUIRE(elevator.current_state() == Elevated::ElevatorState::State::Stopped);
                REQUIRE_FALSE(elevator.time_until_next_event().has_value());
            }
        }

        WHEN("Given target of different floor is set before doors are closed") {
            elevator.set_target(10);

            auto steps = elevator.time_until_next_event();
            REQUIRE(steps.has_value());
            auto result = elevator.update(steps.value());

            THEN("Does not target floor and stays stopped") {
                REQUIRE(result == Elevated::ElevatorState::ElevatorUpdateResult::DoorsClosed);
                REQUIRE(elevator.current_state() == Elevated::ElevatorState::State::Travelling);
                REQUIRE(elevator.time_until_next_event().has_value());
                REQUIRE(elevator.target_height() == 10);
            }
        }
    }

    GIVEN("An elevator filled with passengers") {

        auto generate_filled_elevator = [&](std::initializer_list<Elevated::ElevatorState::TravellingPassenger> passenger_list) {
            Elevated::ElevatorState elevator {0, 0, 0};
            elevator.set_target(0);

            REQUIRE(elevator.current_state() == Elevated::ElevatorState::State::DoorsOpening);
            auto steps = elevator.time_until_next_event();
            REQUIRE(steps.has_value());
            auto result = elevator.update(steps.value());
            REQUIRE(result == Elevated::ElevatorState::ElevatorUpdateResult::DoorsOpened);
            REQUIRE(elevator.current_state() == Elevated::ElevatorState::State::DoorsOpen);
            REQUIRE_FALSE(elevator.time_until_next_event().has_value());
            std::vector<Elevated::Passenger> line;
            for (auto [id, to] : passenger_list)
                line.push_back(Elevated::Passenger{id, 0, to, 0});

            elevator.transfer_passengers(line);
            REQUIRE(elevator.passengers().size() == passenger_list.size());
            REQUIRE(elevator.current_state() == Elevated::ElevatorState::State::DoorsClosing);
            REQUIRE(elevator.passengers().empty());

            return elevator;
        };

        WHEN("The doors are opened on that floor") {

        }
    }
}
