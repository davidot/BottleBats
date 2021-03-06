#include <catch2/catch.hpp>
#include <elevated/Types.h>
#include "elevated/Elevator.h"

using namespace Elevated;

TEST_CASE("Elevators state", "[elevators][state]") {
    GIVEN("An initialized elevator") {
        Elevated::ElevatorID id = GENERATE(0, 2);
        Elevated::GroupID group_id = GENERATE(0, 2);
        Elevated::Height initial_height = GENERATE(0, 2);
        Elevated::Capacity max_capacity = GENERATE(0, 1);
        Elevated::Height speed = GENERATE(1, 2);
        CAPTURE(id, group_id, max_capacity, speed, initial_height);
        Elevated::ElevatorState elevator{id, {group_id, max_capacity, speed}, initial_height};

        THEN("It is initialized properly, stopped, at given height and target and empty") {
            REQUIRE(elevator.speed == speed);
            REQUIRE(elevator.max_capacity == max_capacity);
            REQUIRE(elevator.group_id == group_id);
            REQUIRE(elevator.id == id);

            REQUIRE(elevator.passengers().empty());

            REQUIRE(elevator.height() == initial_height);
            REQUIRE(elevator.target_height() == initial_height);

            REQUIRE(elevator.current_state() == Elevated::ElevatorState::State::Stopped);
        }

        WHEN("It is updated") {
            auto result = elevator.update(GENERATE(0u, 1u, 5u, 100u, 250u));
            THEN("Nothing happened") {
                REQUIRE(result == Elevated::ElevatorState::ElevatorUpdateResult::Nothing);
                REQUIRE(elevator.passengers().empty());
                REQUIRE(elevator.height() == initial_height);
                REQUIRE(elevator.target_height() == initial_height);
            }
        }
    }


    GIVEN("An elevator starting on 0 height floor") {
        Elevated::Height initial_height = GENERATE(0, 2, 20);
        CAPTURE(initial_height);
        Elevated::ElevatorState elevator{0, {0}, initial_height};

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
        Elevated::ElevatorState elevator{0, {0}, initial_height};

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

        Elevated::ElevatorState elevator {0, {group_id}, 0};
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
            auto transferred = elevator.transfer_passengers(line);

            THEN("No passengers were picked up and doors are closing") {
                REQUIRE(line.empty());
                REQUIRE(elevator.passengers().empty());
                REQUIRE(transferred.picked_up_passengers.empty());
                REQUIRE(transferred.dropped_off_passengers.empty());
                REQUIRE(elevator.current_state() == Elevated::ElevatorState::State::DoorsClosing);
                REQUIRE(elevator.time_until_next_event() == elevator.door_closing_time);
            }
        }

        WHEN("Picking up from line with no passengers of matching group id") {
            std::vector<Elevated::Passenger> line;
            auto count = GENERATE(1u, 2u, 10u);
            CAPTURE(count);
            for (auto i = 0u; i < count; ++i)
                line.push_back(Elevated::Passenger{i, {0, 1, other_group_id}});

            REQUIRE(line.size() == count);

            auto transferred = elevator.transfer_passengers(line);

            THEN("No passengers were picked up and doors are closing") {
                CAPTURE(line.size(), elevator.passengers().size());
                REQUIRE(line.size() == count);
                REQUIRE(elevator.passengers().empty());
                REQUIRE(elevator.current_state() == Elevated::ElevatorState::State::DoorsClosing);
                REQUIRE(elevator.time_until_next_event() == elevator.door_closing_time);

                REQUIRE(transferred.picked_up_passengers.empty());
                REQUIRE(transferred.dropped_off_passengers.empty());

                REQUIRE(std::is_sorted(line.begin(), line.end(), [&](Elevated::Passenger const& lhs, Elevated::Passenger const& rhs) {
                    return lhs.id < rhs.id;
                }));
            }
        }

        WHEN("Picking up from line with only passengers of matching group id") {
            std::vector<Elevated::Passenger> line;
            auto count = GENERATE(1u, 2u, 10u);
            CAPTURE(count);
            for (auto i = 0u; i < count; ++i)
                line.push_back(Elevated::Passenger{i, {0, 1, group_id}});

            auto transferred = elevator.transfer_passengers(line);

            THEN("Picks up all passengers and doors are closing") {
                REQUIRE(line.empty());
                REQUIRE(elevator.passengers().size() == count);

                REQUIRE(transferred.picked_up_passengers.size() == count);
                REQUIRE(transferred.dropped_off_passengers.empty());

                for (auto i = 0u; i < count; ++i) {
                    CAPTURE(i);
                    REQUIRE(std::find_if(elevator.passengers().begin(), elevator.passengers().end(),
                                 [&](Elevated::ElevatorState::TravellingPassenger const &p) {
                                     return p.id == i;
                                 }) != elevator.passengers().end());

                    REQUIRE(std::find_if(transferred.picked_up_passengers.begin(), transferred.picked_up_passengers.end(),
                                [&](Elevated::Passenger const &p) {
                                    return p.id == i;
                                }) != transferred.picked_up_passengers.end());
                }

                REQUIRE(elevator.current_state() == Elevated::ElevatorState::State::DoorsClosing);
                REQUIRE(elevator.time_until_next_event() == elevator.door_closing_time);
            }
        }

        WHEN("Picking up from a mixed line of waiting passengers") {
            std::vector<Elevated::Passenger> line {
                {0, {0, 1, group_id}},
                {1, {0, 1, other_group_id}},
                {2, {0, 1, other_group_id}},
                {3, {0, 1, group_id}},
                {4, {0, 1, group_id}},
                {5, {0, 1, other_group_id}},
                {6, {0, 1, other_group_id}},
            };

            auto transferred = elevator.transfer_passengers(line);

            THEN("Picks up all passengers and doors are closing") {
                REQUIRE(line.size() == 4);
                REQUIRE(elevator.passengers().size() == 3);
                REQUIRE(transferred.dropped_off_passengers.empty());

                for (auto i : {0u, 3u, 4u}) {
                    CAPTURE(i);
                    REQUIRE(std::find_if(elevator.passengers().begin(), elevator.passengers().end(),
                                         [&](Elevated::ElevatorState::TravellingPassenger const &p) {
                                             return p.id == i;
                                         }) != elevator.passengers().end());

                    REQUIRE(std::find_if(transferred.picked_up_passengers.begin(), transferred.picked_up_passengers.end(),
                                [&](Elevated::Passenger const &p) {
                                    return p.id == i;
                                }) != transferred.picked_up_passengers.end());
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
        Elevated::ElevatorState elevator {0, {0}, 0};
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
            auto transferred = elevator.transfer_passengers(line);
            REQUIRE(transferred.dropped_off_passengers.empty());
            REQUIRE(transferred.picked_up_passengers.empty());
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

        auto generate_filled_elevator = [&](std::initializer_list<Elevated::ElevatorState::TravellingPassenger> passenger_list, Elevated::Height get_in_floor = 0) {
            CAPTURE(get_in_floor);
            CAPTURE(passenger_list);
            Elevated::ElevatorState elevator {0, {0}, get_in_floor};
            elevator.set_target(get_in_floor);

            REQUIRE(elevator.current_state() == Elevated::ElevatorState::State::DoorsOpening);
            auto steps = elevator.time_until_next_event();
            REQUIRE(steps.has_value());
            auto result = elevator.update(steps.value());
            REQUIRE(result == Elevated::ElevatorState::ElevatorUpdateResult::DoorsOpened);
            REQUIRE(elevator.current_state() == Elevated::ElevatorState::State::DoorsOpen);

            std::vector<Elevated::Passenger> line;
            for (auto [id, to, _] : passenger_list) {
                REQUIRE(get_in_floor != to);
                line.push_back(Elevated::Passenger { id, {get_in_floor, to, 0 }});
            }

            auto transferred = elevator.transfer_passengers(line);
            REQUIRE(line.empty());
            REQUIRE(elevator.passengers().size() == passenger_list.size());
            REQUIRE(elevator.current_state() == Elevated::ElevatorState::State::DoorsClosing);
            REQUIRE(transferred.picked_up_passengers.size() == passenger_list.size());
            REQUIRE(transferred.dropped_off_passengers.empty());

            auto steps_to_close = elevator.time_until_next_event();
            REQUIRE(steps_to_close.has_value());
            auto result_closing = elevator.update(steps_to_close.value());
            REQUIRE(result_closing == Elevated::ElevatorState::ElevatorUpdateResult::DoorsClosed);

            return elevator;
        };

        auto move_to_floor_and_transfer_with_empty = [&](Elevated::ElevatorState& elevator, Elevated::Height target) {
            elevator.set_target(target);
            auto steps = elevator.time_until_next_event();
            REQUIRE(steps.has_value());
            auto result = elevator.update(steps.value());
            REQUIRE(result == Elevated::ElevatorState::ElevatorUpdateResult::DoorsOpened);
            std::vector<Elevated::Passenger> empty_line;
            auto transferred = elevator.transfer_passengers(empty_line);
            REQUIRE(empty_line.empty());
            REQUIRE(transferred.picked_up_passengers.empty());
            return transferred;
        };

        auto elevator_has_passengers_with_id = [](Elevated::ElevatorState const& elevator, std::initializer_list<Elevated::ElevatorState::TravellingPassenger> passengers) {
            for (auto passenger : passengers) {
                auto id = passenger.id;
                auto to = passenger.to;
                CAPTURE(id, to);
                REQUIRE(std::find_if(elevator.passengers().begin(), elevator.passengers().end(),
                            [&](Elevated::ElevatorState::TravellingPassenger const &p) {
                                return p.id == id && p.to == to;
                            }) != elevator.passengers().end());
            }
            REQUIRE(elevator.passengers().size() == passengers.size());
        };

        WHEN("Elevator transfers passengers on floor with passengers with that as a destination") {
            auto elevator = generate_filled_elevator({{1, 0, 0}, {2, 0, 0}}, 1);
            auto transferred = move_to_floor_and_transfer_with_empty(elevator, 0);

            THEN("All passengers got off and did not join the line") {
                REQUIRE(elevator.passengers().size() == 0);
                REQUIRE(transferred.dropped_off_passengers.size() == 2);
                for (auto passenger : {Elevated::ElevatorState::TravellingPassenger{1, 0, 0}, {2, 0, 0}}) {
                    CAPTURE(passenger.id, passenger.to);
                    REQUIRE(std::find_if(transferred.dropped_off_passengers.begin(), transferred.dropped_off_passengers.end(),
                                [&](Elevated::ElevatorID id) {
                                    return id == passenger.id;
                                }) != transferred.dropped_off_passengers.end());
                }
            }
        }

        WHEN("Elevator has no passengers with the target destination floor") {
            auto elevator = generate_filled_elevator({{1, 2, 0}, {2, 3, 0}, {3, 4, 0}, {4, 6, 0}}, 0);
            auto transferred = move_to_floor_and_transfer_with_empty(elevator, GENERATE(0, 1, 5));

            THEN("None got off the elevator") {
                elevator_has_passengers_with_id(elevator, {{1, 2, 0}, {2, 3, 0}, {3, 4, 0}, {4, 6, 0}});
                REQUIRE(transferred.dropped_off_passengers.empty());
            }
        }

        WHEN("Elevator some passengers with the target destination floor") {
            auto elevator = generate_filled_elevator({{1, 2, 0}, {2, 3, 0}, {3, 4, 0}, {4, 6, 0}}, 0);
            auto transferred = move_to_floor_and_transfer_with_empty(elevator, 4);

            THEN("None got off the elevator") {
                elevator_has_passengers_with_id(elevator, {{1, 2, 0}, {2, 3, 0}, {4, 6, 0}});
                for (auto passenger : {Elevated::ElevatorState::TravellingPassenger{3, 4, 0}}) {
                    CAPTURE(passenger.id, passenger.to);
                    REQUIRE(std::find_if(transferred.dropped_off_passengers.begin(), transferred.dropped_off_passengers.end(),
                                [&](Elevated::ElevatorID id) {
                                    return id == passenger.id;
                                }) != transferred.dropped_off_passengers.end());
                }
            }
        }
    }


    GIVEN("An elevator with a capacity limit") {
        GroupID group_id = GENERATE(0, 2, 3);
        GroupID other_group_id = GENERATE(1, 4);
        Capacity max_capacity = 5;
        CAPTURE(group_id, other_group_id);

        ElevatorState elevator {0, {group_id, max_capacity}, 0};
        elevator.set_target(0);

        REQUIRE(elevator.current_state() == ElevatorState::State::DoorsOpening);
        auto steps = elevator.time_until_next_event();
        REQUIRE(steps.has_value());
        auto result = elevator.update(steps.value());
        REQUIRE(result == ElevatorState::ElevatorUpdateResult::DoorsOpened);
        REQUIRE(elevator.current_state() == ElevatorState::State::DoorsOpen);
        REQUIRE_FALSE(elevator.time_until_next_event().has_value());

        WHEN("Picking up passengers with capacity") {
            std::vector<Passenger> line {
                {1, {0, 1, group_id, 1}},
                {2, {0, 1, group_id, 1}},
                {3, {0, 1, group_id, 1}},
                {4, {0, 1, group_id, 1}},
                {5, {0, 1, group_id, 1}},
                {6, {0, 1, group_id, 1}},
                {7, {0, 1, group_id, 1}},
            };

            REQUIRE(line.size() > max_capacity);

            auto transferred = elevator.transfer_passengers(line);

            THEN("Only picks up to the limit") {
                REQUIRE(line.size() == 2);
                REQUIRE(elevator.passengers().size() == 5);
                REQUIRE(transferred.picked_up_passengers.size() == 5);
                REQUIRE(transferred.dropped_off_passengers.empty());

#define REQUIRE_PICKED_UP(...) \
                for (auto i : { __VA_ARGS__ }) { \
                    CAPTURE(i); \
                    REQUIRE(std::find_if(elevator.passengers().begin(), elevator.passengers().end(), \
                                [&](ElevatorState::TravellingPassenger const &p) { return p.id == i; }) != elevator.passengers().end()); \
                                                    \
                    REQUIRE(std::find_if(transferred.picked_up_passengers.begin(), transferred.picked_up_passengers.end(), \
                                [&](Passenger const &p) { return p.id == i; }) != transferred.picked_up_passengers.end()); \
                }
#define REQUIRE_LEFT_IN_LINE(...) \
                for (auto i : { __VA_ARGS__ }) { \
                    CAPTURE(i); \
                    REQUIRE(std::find_if(line.begin(), line.end(), \
                                [&](Passenger const &p) { return p.id == i; }) != line.end()); \
                } \
                REQUIRE(std::is_sorted(line.begin(), line.end(),  \
                    [&](Passenger const& lhs, Passenger const& rhs) { return lhs.id < rhs.id; }))

                REQUIRE_PICKED_UP(1u, 2u, 3u, 4u, 5u)
                REQUIRE_LEFT_IN_LINE(6u, 7u);
            }
        }

        WHEN("Picking up passengers with other groups") {

            std::vector<Passenger> line {
                {1, {0, 1, group_id, max_capacity - 1}},
                {2, {0, 1, other_group_id, 2}},
                {3, {0, 1, other_group_id, 1}},
                {4, {0, 1, group_id, 2}},
                {5, {0, 1, other_group_id, 0}},
                {6, {0, 1, group_id, 1}},
                {7, {0, 1, group_id, 1}},
            };

            auto transferred = elevator.transfer_passengers(line);

            THEN("Did not pick up any from the other group") {
                REQUIRE_PICKED_UP(1u, 6u);
                REQUIRE_LEFT_IN_LINE(2u, 3u, 4u, 5u, 7u);
            }
        }

        WHEN("Picking up passengers capacity 0") {

            std::vector<Passenger> line {
                {1, {0, 1, group_id, max_capacity - 1}},
                {2, {0, 1, group_id, 2}},
                {3, {0, 1, group_id, 0}},
                {4, {0, 1, group_id, 1}},
                {5, {0, 1, group_id, 0}},
                {6, {0, 1, group_id, 1}},
                {7, {0, 1, group_id, 0}},
            };

            auto transferred = elevator.transfer_passengers(line);

            THEN("Can fit all capacity 0 even if completely filled") {
                REQUIRE_PICKED_UP(1u, 3u, 4u, 5u, 7u);
                REQUIRE_LEFT_IN_LINE(2u, 6u);
            }
        }
    }

    GIVEN("An elevator waiting at a floor") {
        Elevated::GroupID group_id = GENERATE(0, 2, 3);
        Elevated::GroupID other_group_id = GENERATE(1, 4);
        CAPTURE(group_id, other_group_id);

        Elevated::ElevatorState elevator {0, {group_id}, 0};
        elevator.set_target(0);

        REQUIRE(elevator.current_state() == Elevated::ElevatorState::State::DoorsOpening);
        auto steps = elevator.time_until_next_event();
        REQUIRE(steps.has_value());
        auto result = elevator.update(steps.value());
        REQUIRE(result == Elevated::ElevatorState::ElevatorUpdateResult::DoorsOpened);
        REQUIRE(elevator.current_state() == Elevated::ElevatorState::State::DoorsOpen);
        REQUIRE_FALSE(elevator.time_until_next_event().has_value());

        std::vector<Elevated::Passenger> line {
            {1, {0, 1, group_id}},
            {2, {0, 1, other_group_id}},
            {3, {0, 1, other_group_id}},
            {4, {0, 1, group_id}},
            {5, {0, 1, group_id}},
            {6, {0, 1, other_group_id}},
            {7, {0, 1, other_group_id}},
        };

        WHEN("Transferring passengers with false callback") {
            std::vector<PassengerID> passenger_ids;

            auto transferred = elevator.transfer_passengers(line, [&](Passenger const& passenger) {
                passenger_ids.push_back(passenger.id);
                return false;
            });

            THEN("Picks up all passengers and doors are closing") {
                REQUIRE(line.size() == 7);
                REQUIRE(elevator.passengers().empty());
                REQUIRE(transferred.dropped_off_passengers.empty());

                REQUIRE(elevator.current_state() == Elevated::ElevatorState::State::DoorsClosing);
                REQUIRE(elevator.time_until_next_event() == elevator.door_closing_time);

                REQUIRE(std::is_sorted(line.begin(), line.end(), [&](Elevated::Passenger const& lhs, Elevated::Passenger const& rhs) {
                    return lhs.id < rhs.id;
                }));

                REQUIRE(passenger_ids == std::vector<PassengerID>{1, 4, 5});
            }
        }

        WHEN("Transferring passengers with false callback") {
            std::vector<PassengerID> passenger_ids;

            auto transferred = elevator.transfer_passengers(line, [&](Passenger const& passenger) {
                passenger_ids.push_back(passenger.id);
                return passenger.id == 4;
            });

            THEN("Picks up all passengers and doors are closing") {
                REQUIRE(line.size() == 6);
                REQUIRE(elevator.passengers().size() == 1);
                REQUIRE(transferred.dropped_off_passengers.empty());

                REQUIRE(elevator.current_state() == Elevated::ElevatorState::State::DoorsClosing);
                REQUIRE(elevator.time_until_next_event() == elevator.door_closing_time);

                for (auto i : {4u}) {
                    CAPTURE(i);
                    REQUIRE(std::find_if(elevator.passengers().begin(), elevator.passengers().end(),
                                [&](Elevated::ElevatorState::TravellingPassenger const &p) {
                                    return p.id == i;
                                }) != elevator.passengers().end());

                    REQUIRE(std::find_if(transferred.picked_up_passengers.begin(), transferred.picked_up_passengers.end(),
                                [&](Elevated::Passenger const &p) {
                                    return p.id == i;
                                }) != transferred.picked_up_passengers.end());
                }

                REQUIRE(std::is_sorted(line.begin(), line.end(), [&](Elevated::Passenger const& lhs, Elevated::Passenger const& rhs) {
                    return lhs.id < rhs.id;
                }));

                REQUIRE(passenger_ids == std::vector<PassengerID>{1, 4, 5});
            }
        }

        WHEN("Transferring passengers with false callback") {
            std::vector<PassengerID> passenger_ids;

            auto transferred = elevator.transfer_passengers(line, [&](Passenger const& passenger) {
                passenger_ids.push_back(passenger.id);
                return passenger.id != 4;
            });

            THEN("Picks up all passengers and doors are closing") {
                REQUIRE(line.size() == 5);
                REQUIRE(elevator.passengers().size() == 2);
                REQUIRE(transferred.dropped_off_passengers.empty());

                REQUIRE(elevator.current_state() == Elevated::ElevatorState::State::DoorsClosing);
                REQUIRE(elevator.time_until_next_event() == elevator.door_closing_time);

                for (auto i : {1u, 5u}) {
                    CAPTURE(i);
                    REQUIRE(std::find_if(elevator.passengers().begin(), elevator.passengers().end(),
                                [&](Elevated::ElevatorState::TravellingPassenger const &p) {
                                    return p.id == i;
                                }) != elevator.passengers().end());

                    REQUIRE(std::find_if(transferred.picked_up_passengers.begin(), transferred.picked_up_passengers.end(),
                                [&](Elevated::Passenger const &p) {
                                    return p.id == i;
                                }) != transferred.picked_up_passengers.end());
                }

                REQUIRE(std::is_sorted(line.begin(), line.end(), [&](Elevated::Passenger const& lhs, Elevated::Passenger const& rhs) {
                    return lhs.id < rhs.id;
                }));

                REQUIRE(passenger_ids == std::vector<PassengerID>{1, 4, 5});
            }
        }
    }
}
