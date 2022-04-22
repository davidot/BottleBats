#pragma once

#include "Elevator.h"
#include "Types.h"
#include "stats/Listener.h"
#include "generation/Generation.h"
#include <optional>
#include <unordered_map>
#include <vector>

namespace Elevated {

class BuildingState {
public:
    explicit BuildingState(BuildingBlueprint blueprint);

    std::optional<Time> next_event_at() const;

    void add_request(PassengerBlueprint passenger);
    void send_elevator(ElevatorID, Height target);
    void update_until(Time target_time);

    void add_listener(std::shared_ptr<EventListener> listener) { m_distributor.add_listener(std::move(listener)); }
    bool remove_listener(EventListener* listener) { return m_distributor.remove_listener(listener); }

private:
    std::unordered_map<Height, std::vector<Passenger>> m_floors;
    std::vector<ElevatorState> m_elevators;
    std::vector<std::unordered_set<Height>> m_group_reachable;

    Time m_current_time{0};
    EventDistributor m_distributor;
    PassengerID m_next_passenger_id{1};
};

}
