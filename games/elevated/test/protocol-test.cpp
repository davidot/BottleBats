#include <catch2/catch.hpp>
#include <elevated/Types.h>
#include <elevated/algorithm/ProcessAlgorithm.h>

using namespace Elevated;

TEST_CASE("Text based protocol for process algorithm", "[protocol]") {

    SECTION("Initial building message") {
        BuildingBlueprint building {
            { { 0u, 5u, 10u, 15u }, { 5u, 15u } },
            { { 0 }, { 1 } }
        };

        std::ostringstream str;
        ProcessAlgorithm::write_building(building, str);

        REQUIRE(str.str() == "building 2 2\n"
                     "group 0 4 0,5,10,15\n"
                     "group 1 2 5,15\n"
                     "elevator 0 0 1 1 1 1\n"
                     "elevator 1 1 1 1 1 1\n");
    }

    SECTION("Elevator closed message") {
        GIVEN("An elevator and algorithm at level Low") {
            ProcessAlgorithm algorithm { { "!do not run!" }, Elevated::ProcessAlgorithm::InfoLevel::Low };

            ElevatorID elevator_id = GENERATE(0u, 1u);
            GroupID group_id = GENERATE(0u, 1u);
            Height height = GENERATE(0u, 1u);
            CAPTURE(elevator_id, group_id, height);

            auto base_string = std::to_string(elevator_id) + " " + std::to_string(group_id) + " " + std::to_string(height) + " ";

            ElevatorState elevator { elevator_id, group_id, 1, height };
            elevator.set_target(height);
            REQUIRE(elevator.time_until_next_event().has_value());
            elevator.update(elevator.time_until_next_event().value());
            REQUIRE(elevator.current_state() == Elevated::ElevatorState::State::DoorsOpen);

            WHEN("Picking up no passengers") {
                std::vector<Passenger> line {};
                elevator.transfer_passengers(line);
                REQUIRE(elevator.time_until_next_event().has_value());
                auto result = elevator.update(elevator.time_until_next_event().value());
                REQUIRE(result == Elevated::ElevatorState::ElevatorUpdateResult::DoorsClosed);
                REQUIRE(elevator.passengers().empty());

                THEN("Has empty passenger message") {
                    std::ostringstream str;
                    algorithm.write_elevator_base(elevator, str);
                    REQUIRE(str.str() == base_string + "0 -");
                }
            }
            
            WHEN("Picking up a passenger") {
                Height target_height = height + 1;
                std::vector<Passenger> line {
                    {1, {height, target_height, group_id, 0}}
                };
                elevator.transfer_passengers(line);
                REQUIRE(elevator.time_until_next_event().has_value());
                auto result = elevator.update(elevator.time_until_next_event().value());
                REQUIRE(result == Elevated::ElevatorState::ElevatorUpdateResult::DoorsClosed);
                REQUIRE(elevator.passengers().size() == 1);
                
                THEN("Has single passenger message") {
                    std::ostringstream str;
                    algorithm.write_elevator_base(elevator, str);
                    REQUIRE(str.str() == base_string + "1 "
                            + std::to_string(target_height));
                }
            }

            WHEN("Picking up multiple passengers with same target") {
                Height target_height = height + 1;
                std::vector<Passenger> line {
                    {1, {height, target_height, group_id, 0}},
                    {2, {height, target_height, group_id, 0}},
                    {3, {height, target_height, group_id, 0}},
                };
                elevator.transfer_passengers(line);
                REQUIRE(elevator.time_until_next_event().has_value());
                auto result = elevator.update(elevator.time_until_next_event().value());
                REQUIRE(result == Elevated::ElevatorState::ElevatorUpdateResult::DoorsClosed);
                REQUIRE(elevator.passengers().size() == 3);

                THEN("Has single target message") {
                    std::ostringstream str;
                    algorithm.write_elevator_base(elevator, str);
                    REQUIRE(str.str() == base_string + "1 "
                            + std::to_string(target_height));
                }
            }

            WHEN("Picking up multiple passengers with different targets") {
                Height target_height1 = height + 1;
                Height target_height2 = height + 2;
                std::vector<Passenger> line {
                    {1, {height, target_height1, group_id, 0}},
                    {2, {height, target_height2, group_id, 0}},
                };
                elevator.transfer_passengers(line);
                REQUIRE(elevator.time_until_next_event().has_value());
                auto result = elevator.update(elevator.time_until_next_event().value());
                REQUIRE(result == Elevated::ElevatorState::ElevatorUpdateResult::DoorsClosed);
                REQUIRE(elevator.passengers().size() == 2);

                THEN("Has multi target message") {
                    std::ostringstream str;
                    algorithm.write_elevator_base(elevator, str);
                    REQUIRE(str.str() == base_string + "2 "
                            + std::to_string(target_height1) + ","
                            + std::to_string(target_height2));
                }
            }

            WHEN("Picking up multiple passengers with different targets with higher first") {
                Height target_height1 = height + 1;
                Height target_height2 = height + 2;
                std::vector<Passenger> line {
                    {1, {height, target_height2, group_id, 0}},
                    {2, {height, target_height1, group_id, 0}},
                };
                elevator.transfer_passengers(line);
                REQUIRE(elevator.time_until_next_event().has_value());
                auto result = elevator.update(elevator.time_until_next_event().value());
                REQUIRE(result == Elevated::ElevatorState::ElevatorUpdateResult::DoorsClosed);
                REQUIRE(elevator.passengers().size() == 2);

                THEN("Target still go from low to high") {
                    std::ostringstream str;
                    algorithm.write_elevator_base(elevator, str);
                    REQUIRE(str.str() == base_string + "2 "
                            + std::to_string(target_height1) + ","
                            + std::to_string(target_height2));
                }
            }

            WHEN("Picking up multiple passengers with different targets") {
                Height target_height1 = height + 1;
                Height target_height2 = height + 2;
                Height target_height3 = height + 3;
                std::vector<Passenger> line {
                    {1, {height, target_height3, group_id, 0}},
                    {2, {height, target_height1, group_id, 0}},
                    {3, {height, target_height1, group_id, 0}},
                    {4, {height, target_height1, group_id, 0}},
                    {5, {height, target_height2, group_id, 0}},
                    {6, {height, target_height3, group_id, 0}},
                };
                elevator.transfer_passengers(line);
                REQUIRE(elevator.time_until_next_event().has_value());
                auto result = elevator.update(elevator.time_until_next_event().value());
                REQUIRE(result == Elevated::ElevatorState::ElevatorUpdateResult::DoorsClosed);
                REQUIRE(elevator.passengers().size() == 6);

                THEN("Target still go from low to high") {
                    std::ostringstream str;
                    algorithm.write_elevator_base(elevator, str);
                    REQUIRE(str.str() == base_string + "3 "
                            + std::to_string(target_height1) + ","
                            + std::to_string(target_height2) + ","
                            + std::to_string(target_height3));
                }
            }
        }
    }


    SECTION("New request message") {
        GIVEN("An algorithm at level Low") {
            ProcessAlgorithm algorithm { { "!do not run!" }, Elevated::ProcessAlgorithm::InfoLevel::Low };

            WHEN("Passenger is going up") {
                Height from = GENERATE(0u, 1u);
                Height to = GENERATE(2u, 3u);
                GroupID group = GENERATE(0u, 1u);
                Passenger passenger{GENERATE(1u, 2u, 3u), {from, to, group, GENERATE(0u, 1u)}};

                THEN("Message has up") {
                    std::ostringstream str;
                    algorithm.write_new_request(passenger, str);
                    REQUIRE(str.str() == std::to_string(from) + ' ' + std::to_string(group) + " up");
                }
            }

            WHEN("Passenger is going down") {
                Height from = GENERATE(2u, 3u);
                Height to = GENERATE(0u, 1u);
                GroupID group = GENERATE(0u, 1u);
                Passenger passenger{GENERATE(1u, 2u, 3u), {from, to, group, GENERATE(0u, 1u)}};

                THEN("Message has down") {
                    std::ostringstream str;
                    algorithm.write_new_request(passenger, str);
                    REQUIRE(str.str() == std::to_string(from) + ' ' + std::to_string(group) + " down");
                }
            }
        }
    }

}
