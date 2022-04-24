#include <catch2/catch.hpp>
#include <elevated/Types.h>
#include "elevated/generation/Generation.h"
#include "elevated/Building.h"

using namespace Elevated;

class StoringEventListener : public EventListener {
public:

    std::vector<std::pair<Time, Passenger>> request_created_events;
    void on_request_created(Time at, Passenger const& passenger) override
    {
        request_created_events.emplace_back(at, passenger);
    }

    std::vector<std::tuple<Time, Passenger, ElevatorID>> passenger_enter_events;
    void on_passenger_enter_elevator(Time at, Passenger const& passenger, ElevatorID id) override
    {
        passenger_enter_events.emplace_back(at, passenger, id);
    }

    std::vector<std::tuple<Time, PassengerID, Height>> passenger_leave_events;
    void on_passenger_leave_elevator(Time at, PassengerID id, Height height) override
    {
        passenger_leave_events.emplace_back(at, id, height);
    }

    std::vector<std::pair<Time, ElevatorState>> elevator_opened_events;
    void on_elevator_opened_doors(Time at, ElevatorState const& elevator) override
    {
        elevator_opened_events.emplace_back(at, elevator);
    }

    std::vector<std::pair<Time, ElevatorState>> elevator_closed_events;
    void on_elevator_closed_doors(Time at, ElevatorState const& elevator) override
    {
        elevator_closed_events.emplace_back(at, elevator);
    }

    std::vector<std::tuple<Time, Height, ElevatorState>> elevator_set_target_events;
    void on_elevator_set_target(Time at, Height new_target, ElevatorState const& elevator) override
    {
        elevator_set_target_events.emplace_back(at, new_target, elevator);
    }

    std::vector<std::tuple<Time, Time, ElevatorState>> elevator_stopped_events;
    void on_elevator_stopped(Time at, Time duration, ElevatorState const& elevator) override
    {
        elevator_stopped_events.emplace_back(at, duration, elevator);
    }

    [[nodiscard]] bool no_events() const {
        bool empty = request_created_events.empty()
            && passenger_enter_events.empty()
            && passenger_leave_events.empty()
            && elevator_opened_events.empty()
            && elevator_closed_events.empty()
            && elevator_set_target_events.empty()
            && elevator_stopped_events.empty();

        if (!empty)
            UNSCOPED_INFO(
                    "request: " << request_created_events.size()
                    << ", enter: " << passenger_enter_events.size()
                    << ", leave: " << passenger_leave_events.size()
                    << ", opened: " << elevator_opened_events.size()
                    << ", closed: " << elevator_closed_events.size()
                    << ", target: " << elevator_set_target_events.size()
                    << ", stopped: " << elevator_stopped_events.size()
            );
        return empty;
    }

    void clear_events() {
        request_created_events.clear();
        passenger_enter_events.clear();
        passenger_leave_events.clear();
        elevator_opened_events.clear();
        elevator_closed_events.clear();
        elevator_set_target_events.clear();
        elevator_stopped_events.clear();
    }
};

TEST_CASE("Events", "[building][event]") {

    GIVEN("A building with floors and single elevator") {
        StoringEventListener listener;
        
        BuildingState building {BuildingBlueprint {
            {{0u, 5u, 10u, 15u}},
            {{0}}
        }, &listener};

        REQUIRE(listener.no_events());

        WHEN("The building is updated") {
            Time time = GENERATE(1, 2, 5, 10, 20);

            building.update_until(time);

            THEN("Elevator stopped events are generated") {
                REQUIRE(listener.elevator_stopped_events.size() == 1);
                auto event = listener.elevator_stopped_events.front();
                REQUIRE(std::get<0>(event) == time);
                REQUIRE(std::get<1>(event) == time);

                REQUIRE(std::get<2>(event).id == 0);
                REQUIRE(std::get<2>(event).group_id == 0);
            }
        }

        WHEN("The building is updated twice") {
            Time first_time = GENERATE(1, 2, 5, 10, 20);
            Time second_time = GENERATE(1, 2, 5, 10, 20);
            Time total_time = first_time + second_time;

            building.update_until(first_time);
            listener.clear_events();

            building.update_until(total_time);


            THEN("Elevator stopped events are generated with specified durations") {
                REQUIRE(listener.elevator_stopped_events.size() == 1);
                auto event = listener.elevator_stopped_events.front();
                REQUIRE(std::get<0>(event) == total_time);
                REQUIRE(std::get<1>(event) == second_time);

                REQUIRE(std::get<2>(event).id == 0);
                REQUIRE(std::get<2>(event).group_id == 0);
            }
        }

        WHEN("A request is generated on any floor") {
            PassengerBlueprint request {GENERATE(0u, 5u, 10u), 15, 0};

            Time time = GENERATE(0, 1, 10);
            if (time != 0)
                building.update_until(time);

            listener.clear_events();

            building.add_request(request);

            THEN("One request event is generated") {
                REQUIRE(listener.request_created_events.size() == 1);
                auto event = listener.request_created_events.front();
                REQUIRE(event.first == time);
                REQUIRE(event.second.from == request.from);
                REQUIRE(event.second.to == request.to);
                REQUIRE(event.second.group == request.group);
                listener.request_created_events.clear();
                REQUIRE(listener.no_events());
            }
        }
        
        WHEN("A target is set on the elevator") {
            Time initial_time = GENERATE(0u, 1u, 5u);
            if (initial_time > 0)
                building.update_until(initial_time);
            listener.clear_events();

            Height target = GENERATE(0u, 5u, 10u, 15u);
            CAPTURE(target);
            building.send_elevator(0, target);

            THEN("A elevator target event is generated") {
                REQUIRE(listener.elevator_set_target_events.size() == 1);
                auto& [time, height, elevatorState] = listener.elevator_set_target_events.front();
                REQUIRE(time == initial_time);
                REQUIRE(height == target);
                REQUIRE(elevatorState.group_id == 0);
                REQUIRE(elevatorState.id == 0);
                REQUIRE(elevatorState.target_height() == elevatorState.height());
                listener.elevator_set_target_events.clear();
                REQUIRE(listener.no_events());
            }
        }

        WHEN("An elevator opens its doors") {
            Time initial_time = GENERATE(0u, 1u, 5u);
            Height target = GENERATE(0u, 5u, 15u);
            CAPTURE(initial_time, target);
            if (initial_time > 0)
                building.update_until(initial_time);
            building.send_elevator(0, target);
            listener.clear_events();

            auto open_time = building.next_event_at();
            REQUIRE(open_time.has_value());
            building.update_until(open_time.value());

            THEN("An elevator opened door event was generated") {
                REQUIRE(listener.elevator_opened_events.size() == 1);
                auto& [time, elevatorState] = listener.elevator_opened_events.front();
                REQUIRE(time == open_time.value());
                REQUIRE(elevatorState.group_id == 0);
                REQUIRE(elevatorState.id == 0);
                REQUIRE(elevatorState.target_height() == elevatorState.height());
                REQUIRE(elevatorState.height() == target);
                REQUIRE(elevatorState.current_state() == ElevatorState::State::DoorsOpen);
                listener.elevator_opened_events.clear();
                REQUIRE(listener.no_events());
            }
        }

        WHEN("An elevator closes its doors") {
            Time initial_time = GENERATE(0u, 1u, 5u);
            Height target = GENERATE(0u, 5u, 15u);
            CAPTURE(initial_time, target);
            if (initial_time > 0)
                building.update_until(initial_time);
            building.send_elevator(0, target);

            auto open_time = building.next_event_at();
            REQUIRE(open_time.has_value());
            building.update_until(open_time.value());
            REQUIRE(listener.elevator_opened_events.size() == 1);
            listener.clear_events();
            auto close_time = building.next_event_at();
            REQUIRE(close_time.has_value());
            building.update_until(close_time.value());

            THEN("An elevator closed door event was generated") {
                REQUIRE(listener.elevator_closed_events.size() == 1);
                auto& [time, elevatorState] = listener.elevator_closed_events.front();
                REQUIRE(time == close_time.value());
                REQUIRE(elevatorState.group_id == 0);
                REQUIRE(elevatorState.id == 0);
                REQUIRE(elevatorState.target_height() == elevatorState.height());
                REQUIRE(elevatorState.height() == target);
                REQUIRE(elevatorState.current_state() == ElevatorState::State::Stopped);
                listener.elevator_closed_events.clear();
                REQUIRE(listener.no_events());
            }
        }

        WHEN("A passenger enters an elevator") {
            Time initial_time = GENERATE(0u, 1u, 5u);
            Height target = GENERATE(0u, 5u, 15u);
            CAPTURE(initial_time, target);
            PassengerBlueprint passenger {
                    target, 10, 0
            };
            building.add_request(passenger);

            if (initial_time > 0)
                building.update_until(initial_time);
            building.send_elevator(0, target);

            listener.clear_events();

            auto open_time = building.next_event_at();
            REQUIRE(open_time.has_value());
            building.update_until(open_time.value());

            THEN("A passenger enter event is generated") {
                REQUIRE(listener.passenger_enter_events.size() == 1);
                auto& [time, entered_passenger, elevatorID] = listener.passenger_enter_events.front();
                REQUIRE(time == open_time.value());
                REQUIRE(entered_passenger.from == passenger.from);
                REQUIRE(entered_passenger.to == passenger.to);
                REQUIRE(entered_passenger.group == passenger.group);
                REQUIRE(elevatorID == 0);
                listener.passenger_enter_events.clear();
                REQUIRE(listener.elevator_opened_events.size() <= 1);
                listener.elevator_opened_events.clear();

                REQUIRE(listener.no_events());
            }
        }

        WHEN("A passenger enters an elevator") {
            Time initial_time = GENERATE(0u, 1u, 5u);
            Height target = GENERATE(0u, 5u, 15u);
            CAPTURE(initial_time, target);
            PassengerBlueprint passenger {
                    target, 10, 0
            };
            building.add_request(passenger);

            REQUIRE(listener.request_created_events.size() == 1);
            auto generated_passenger_id = listener.request_created_events.front().second.id;

            if (initial_time > 0)
                building.update_until(initial_time);
            building.send_elevator(0, target);

            {
                auto open_time = building.next_event_at();
                REQUIRE(open_time.has_value());
                building.update_until(open_time.value());
            }


            {
                auto close_time = building.next_event_at();
                REQUIRE(close_time.has_value());
                building.update_until(close_time.value());
            }

            building.send_elevator(0, 10);
            listener.clear_events();

            auto open_time = building.next_event_at();
            REQUIRE(open_time.has_value());
            building.update_until(open_time.value());

            THEN("A passenger leave event is generated") {
                REQUIRE(listener.passenger_leave_events.size() == 1);
                auto& [time, passenger_id, left_height] = listener.passenger_leave_events.front();
                REQUIRE(time == open_time.value());
                REQUIRE(passenger_id == generated_passenger_id);
                REQUIRE(left_height == 10);
                listener.passenger_leave_events.clear();
                REQUIRE(listener.elevator_opened_events.size() <= 1);
                listener.elevator_opened_events.clear();

                REQUIRE(listener.no_events());
            }
        }


    }

}
