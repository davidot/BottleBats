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
        return request_created_events.empty()
            && passenger_enter_events.empty()
            && passenger_leave_events.empty()
            && elevator_opened_events.empty()
            && elevator_closed_events.empty()
            && elevator_set_target_events.empty()
            && elevator_stopped_events.empty();
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

TEST_CASE("Events", "[event]") {

    GIVEN("A building with floors") {
        BuildingState building {BuildingBlueprint {
            {{0u, 5u, 10u, 15u}},
            {{0}}
        }};

        std::shared_ptr<StoringEventListener> listener = std::make_shared<StoringEventListener>();
        building.add_listener(listener);

        WHEN("A request is generated on any floor") {
            Passenger request {1, GENERATE(0u, 5u, 10u), 15, 0};

            Time time = GENERATE(0, 1, 10);
            if (time != 0)
                building.update_until(time);

            listener->clear_events();

            building.add_request(request);

            THEN("One request event is generated") {
                REQUIRE(listener->request_created_events.size() == 1);
                auto event = listener->request_created_events.front();
                REQUIRE(event.first == time);
                REQUIRE(event.second == request);
                listener->request_created_events.clear();
                REQUIRE(listener->no_events());
            }
        }

    }

}
