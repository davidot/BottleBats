#include "StoringAlgorithm.h"
#include "StoringEventListener.h"
#include <catch2/catch.hpp>
#include <elevated/Simulation.h>
#include <elevated/Types.h>
#include <elevated/algorithm/CyclingAlgorithm.h>
#include <elevated/generation/FullGenerators.h>

using namespace Elevated;

std::unique_ptr<ScenarioGenerator> hardcoded(std::vector<std::pair<size_t, std::vector<Height>>> building, std::vector<std::pair<size_t, std::vector<PassengerBlueprint>>> requests, bool hide_errors = false)
{
    return std::make_unique<HardcodedScenarioGenerator>(std::move(building), std::move(requests), 1, hide_errors);
}

TEST_CASE("Simulator", "[simulator]") {

    GIVEN("A simulation with a failing building generator") {
        Simulation simulation { hardcoded({}, {}), std::make_unique<StoringAlgorithm>() };
        auto listener = std::make_shared<StoringEventListener>();
        simulation.add_listener(listener);

        REQUIRE(dynamic_cast<StoringAlgorithm*>(&simulation.algorithm()));
        auto& algorithm = dynamic_cast<StoringAlgorithm&>(simulation.algorithm());

        WHEN("The simulation is run") {
            auto result = simulation.run();

            THEN("The result is failed generation") {
                REQUIRE(result.type == Elevated::SimulatorResult::Type::GenerationFailed);
                REQUIRE_FALSE(algorithm.got_building());
                REQUIRE(listener->no_events());
            }
        }
    }

    GIVEN("A simulation with a valid building generator but rejecting algorithm") {
        Simulation simulation { hardcoded({ { 1, { 0, 1 } } }, { { 10, { { 0, 1, 0 } } } }), std::make_unique<StoringAlgorithm>(false) };

        auto listener = std::make_shared<StoringEventListener>();
        simulation.add_listener(listener);

        REQUIRE(dynamic_cast<StoringAlgorithm*>(&simulation.algorithm()));
        auto& algorithm = dynamic_cast<StoringAlgorithm&>(simulation.algorithm());

        WHEN("The simulation is run") {
            auto result = simulation.run();

            THEN("The result is rejected run") {
                REQUIRE(result.type == Elevated::SimulatorResult::Type::AlgorithmRejected);
                REQUIRE(algorithm.got_building());
                REQUIRE(listener->no_events());
            }
        }
    }

    GIVEN("A simulation with a valid building generator and accepting algorithm") {
        Simulation simulation { hardcoded({ { 1, { 0, 1 } } }, { { 10, { { 0, 1, 0 } } } }), std::make_unique<StoringAlgorithm>(true) };

        auto listener = std::make_shared<StoringEventListener>();
        simulation.add_listener(listener);

        REQUIRE(dynamic_cast<StoringAlgorithm*>(&simulation.algorithm()));
        auto& algorithm = dynamic_cast<StoringAlgorithm&>(simulation.algorithm());

        WHEN("The simulation is run") {
            auto result = simulation.run();

            THEN("The result is a run which failed because of no algorithm action") {
                REQUIRE(result.type == Elevated::SimulatorResult::Type::NoNextEvent);
                REQUIRE(algorithm.got_building());

                REQUIRE(!algorithm.building_result.value().has_error());

                REQUIRE_FALSE(listener->no_events());
                REQUIRE(algorithm.received_inputs.size() == 1);
            }
        }
    }

    GIVEN("A simulation with a valid building generator and accepting algorithm") {
        Simulation simulation { hardcoded({ { 1, { 0, 1 } } }, { { 10, { { 0, 1, 0 } } } }), std::make_unique<StoringAlgorithm>(true) };

        auto listener = std::make_shared<StoringEventListener>();
        simulation.add_listener(listener);

        REQUIRE(dynamic_cast<StoringAlgorithm*>(&simulation.algorithm()));
        auto& algorithm = dynamic_cast<StoringAlgorithm&>(simulation.algorithm());

        WHEN("The simulation is run") {
            auto result = simulation.run();

            THEN("The result is a run which failed because of no algorithm action") {
                REQUIRE(result.type == Elevated::SimulatorResult::Type::NoNextEvent);
                REQUIRE(algorithm.got_building());

                REQUIRE(!algorithm.building_result.value().has_error());

                REQUIRE_FALSE(listener->no_events());
                REQUIRE(algorithm.received_inputs.size() == 1);
            }
        }
    }

    GIVEN("A simulation with a valid building generator and accepting algorithm with predefined responses") {
        Simulation simulation { hardcoded({ { 1, { 0, 1 } } }, { { 10, { { 0, 1, 0 } } } }), std::make_unique<StoringAlgorithm>(true) };

        auto listener = std::make_shared<StoringEventListener>();
        simulation.add_listener(listener);

        REQUIRE(dynamic_cast<StoringAlgorithm*>(&simulation.algorithm()));
        auto& algorithm = dynamic_cast<StoringAlgorithm&>(simulation.algorithm());

        algorithm.add_response(10, { AlgorithmResponse::move_elevator_to(0, 0) });
        algorithm.add_response(12, { AlgorithmResponse::move_elevator_to(0, 1) });

        WHEN("The simulation is run") {
            auto result = simulation.run();

            THEN("The result is a run which failed because of no algorithm action") {
                REQUIRE(result.type == Elevated::SimulatorResult::Type::SuccessFull);
                REQUIRE(algorithm.got_building());

                REQUIRE(!algorithm.building_result.value().has_error());
                REQUIRE(algorithm.received_inputs.size() == 2);

                REQUIRE_FALSE(listener->no_events());
            }
        }
    }

    GIVEN("A simulation with with time between responses") {
        Simulation simulation { hardcoded({ { 1, { 0, 10 } } }, { { 0, { { 0, 10, 0 } } } }), std::make_unique<StoringAlgorithm>(true) };

        auto listener = std::make_shared<StoringEventListener>();
        simulation.add_listener(listener);

        REQUIRE(dynamic_cast<StoringAlgorithm*>(&simulation.algorithm()));
        auto& algorithm = dynamic_cast<StoringAlgorithm&>(simulation.algorithm());

        WHEN("The simulation is run with timer events spread through out") {
            algorithm.add_response(0, { AlgorithmResponse::set_timer_at(5) });
            algorithm.add_response(5, { AlgorithmResponse::set_timer_at(10), AlgorithmResponse::move_elevator_to(0, 0) });
            algorithm.add_response(7, { AlgorithmResponse::set_timer_at(10), AlgorithmResponse::move_elevator_to(0, 10) });
            algorithm.add_response(10, { AlgorithmResponse::set_timer_at(20), AlgorithmResponse::move_elevator_to(0, 10) });

            auto result = simulation.run();

            THEN("The result is a run which passed with the given timer events having been triggered") {
                REQUIRE(result.type == Elevated::SimulatorResult::Type::SuccessFull);
                REQUIRE(algorithm.got_building());

                REQUIRE(!algorithm.building_result.value().has_error());
                REQUIRE(algorithm.received_inputs.size() == 4);

                {
                    auto first_input = algorithm.received_inputs[0];
                    auto& [time, building, inputs] = first_input;
                    REQUIRE(time == 0);
                    REQUIRE(building.passengers_at(0).size() == 1);
                    REQUIRE(inputs.size() == 1);
                    REQUIRE(inputs[0].type() == Elevated::AlgorithmInput::Type::NewRequestMade);
                }

                {
                    auto& [time, building, inputs] = algorithm.received_inputs[1];
                    REQUIRE(time == 5);
                    REQUIRE(building.passengers_at(0).size() == 1);
                    REQUIRE(inputs.size() == 1);
                    REQUIRE(inputs[0].type() == Elevated::AlgorithmInput::Type::TimerFired);
                }


                REQUIRE_FALSE(listener->no_events());
                REQUIRE(listener->request_created_events.size() == 1);
                REQUIRE(listener->request_created_events[0].first == 1);
            }
        }
    }

    GIVEN("A simulation with with time between responses") {
        Simulation simulation { hardcoded({ { 1, { 0, 10 } } }, { { 0, { { 0, 10, 0 } } } }), std::make_unique<StoringAlgorithm>(true) };

        REQUIRE(dynamic_cast<StoringAlgorithm*>(&simulation.algorithm()));
        auto& algorithm = dynamic_cast<StoringAlgorithm&>(simulation.algorithm());

        algorithm.add_response(0, { AlgorithmResponse::set_timer_at(5) });
        algorithm.add_response(5, { AlgorithmResponse::set_timer_at(2) });

        WHEN("The simulation is run") {
            auto result = simulation.run();

            THEN("The result is a run which failed because of no algorithm action") {
                REQUIRE(result.type == Elevated::SimulatorResult::Type::AlgorithmMisbehaved);
            }
        }
    }

    GIVEN("Requests in wrong order") {
        Simulation simulation { hardcoded({ { 1, { 0, 10 } } }, { { 10, { { 0, 10, 0 } } }, { 0, { { 0, 10, 0 } } } }, true), std::make_unique<StoringAlgorithm>() };

        WHEN("Simulation is run") {
            auto result = simulation.run();

            THEN("Fails with request generation") {
                REQUIRE(result.type == Elevated::SimulatorResult::Type::RequestGenerationFailed);
            }
        }
    }

    GIVEN("Simulation which is never solved") {
        Simulation simulation { hardcoded({ { 1, { 0, 10 } } }, { { 0, { { 0, 10, 0 } } } }), std::make_unique<StoringAlgorithm>() };

        REQUIRE(dynamic_cast<StoringAlgorithm*>(&simulation.algorithm()));
        auto& algorithm = dynamic_cast<StoringAlgorithm&>(simulation.algorithm());
        for (Time t = 0; t < 1000000; t += 1000)
            algorithm.add_response(t, { AlgorithmResponse::set_timer_at(t + 1000) });

        WHEN("Simulation is run") {
            auto result = simulation.run();

            THEN("Fails with did not move all passengers") {
                REQUIRE(result.type == Elevated::SimulatorResult::Type::FailedToResolveAllRequests);
            }
        }
    }

    GIVEN("A simulation with a failing algorithm") {
        Simulation simulation { hardcoded({ { 1, { 0, 10 } } }, { { 0, { { 0, 10, 0 } } } }), std::make_unique<StoringAlgorithm>() };

        REQUIRE(dynamic_cast<StoringAlgorithm*>(&simulation.algorithm()));
        auto& algorithm = dynamic_cast<StoringAlgorithm&>(simulation.algorithm());
        algorithm.add_response(0, { AlgorithmResponse::algorithm_failed({"Oh no", "Whoops"}) });

        WHEN("Simulation is run") {
            auto result = simulation.run();

            THEN("Fails with did not move all passengers") {
                REQUIRE(result.type == Elevated::SimulatorResult::Type::AlgorithmFailed);
                REQUIRE(result.output_messages.size() == 2);
                REQUIRE(result.output_messages[0] == "Oh no");
                REQUIRE(result.output_messages[1] == "Whoops");
            }
        }
    }

    GIVEN("A simulation with a misbehaving algorithm") {
        Simulation simulation { hardcoded({ { 1, { 0, 10 } } }, { { 0, { { 0, 10, 0 } } } }), std::make_unique<StoringAlgorithm>() };

        REQUIRE(dynamic_cast<StoringAlgorithm*>(&simulation.algorithm()));
        auto& algorithm = dynamic_cast<StoringAlgorithm&>(simulation.algorithm());
        algorithm.add_response(0, { AlgorithmResponse::algorithm_misbehaved({"Naughty", "Disable"}) });

        WHEN("Simulation is run") {
            auto result = simulation.run();

            THEN("Fails with did not move all passengers") {
                REQUIRE(result.type == Elevated::SimulatorResult::Type::AlgorithmMisbehaved);
                REQUIRE(result.output_messages.size() == 2);
                REQUIRE(result.output_messages[0] == "Naughty");
                REQUIRE(result.output_messages[1] == "Disable");
            }
        }
    }

    GIVEN("A simulation with the cycling algorithm and simple requests") {
        Simulation simulation { hardcoded({ { 1, { 0, 10 } } },
                                    {
                                        { 0, { { 0, 10, 0 } } },
                                        { 11, { { 0, 10, 0 }, { 10, 0, 0 } } },
                                        { 22, { { 0, 10, 0 } } },
                                        { 33, { { 0, 10, 0 }, { 10, 0, 0 } } },
                                        { 44, { { 0, 10, 0 } } },
                                        { 55, { { 0, 10, 0 }, { 10, 0, 0 } } },
                                        { 66, { { 0, 10, 0 }, { 10, 0, 0 } } },
                                        { 77, { { 0, 10, 0 } } },
                                        { 88, { { 0, 10, 0 }, { 10, 0, 0 } } },
                                        { 99, { { 0, 10, 0 } } },
                                        { 111, { { 10, 0, 0 } } },
                                    }),
            std::make_unique<CyclingAlgorithm>() };

        auto listener = std::make_shared<StoringEventListener>();
        simulation.add_listener(listener);

        WHEN("The simulation is run") {

            auto result = simulation.run();

            THEN("It is successful and got all the passengers") {
                REQUIRE(result.type == Elevated::SimulatorResult::Type::SuccessFull);
                REQUIRE(listener->request_created_events.size() == 16);
                REQUIRE(listener->passenger_enter_events.size() == 16);
                REQUIRE(listener->passenger_leave_events.size() == 16);
            }
        }
    }

    GIVEN("A simulation with the cycling algorithm and simple requests and extra floors") {
        Simulation simulation { hardcoded({ { 1, { 0, 5, 10, 100 } } },
                                    {
                                        { 0, { { 0, 10, 0 } } },
                                        { 10, { { 0, 10, 0 }, { 10, 0, 0 } } },
                                        { 20, { { 0, 10, 0 } } },
                                        { 30, { { 0, 10, 0 }, { 10, 0, 0 } } },
                                        { 40, { { 0, 100, 0 } } },
                                        { 50, { { 0, 100, 0 }, { 10, 0, 0 } } },
                                        { 60, { { 5, 100, 0 }, { 10, 0, 0 } } },
                                        { 70, { { 0, 10, 0 } } },
                                        { 80, { { 0, 10, 0 }, { 10, 0, 0 } } },
                                        { 90, { { 100, 5, 0 } } },
                                        { 100, { { 5, 100, 0 } } },
                                    }),
            std::make_unique<CyclingAlgorithm>() };

        auto listener = std::make_shared<StoringEventListener>();
        simulation.add_listener(listener);

        WHEN("The simulation is run") {
            auto result = simulation.run();

            THEN("It is successful and got all the passengers") {
                REQUIRE(result.type == Elevated::SimulatorResult::Type::SuccessFull);
                REQUIRE(listener->request_created_events.size() == 16);
                REQUIRE(listener->passenger_enter_events.size() == 16);
                REQUIRE(listener->passenger_leave_events.size() == 16);
            }
        }
    }
}
