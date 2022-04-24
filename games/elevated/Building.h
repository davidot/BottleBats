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
    explicit BuildingState(BuildingBlueprint blueprint, EventListener* event_listener);

    BuildingState() = default;

    [[nodiscard]] std::optional<Time> next_event_at() const;

    void add_request(PassengerBlueprint passenger);
    void send_elevator(ElevatorID, Height target);
    void update_until(Time target_time);

    [[nodiscard]] std::vector<Passenger> const& passengers_at(Height) const;
    [[nodiscard]] ElevatorState const& elevator(ElevatorID) const;

private:
    std::unordered_map<Height, std::vector<Passenger>> m_floors;
    std::vector<ElevatorState> m_elevators;
    std::vector<std::unordered_set<Height>> m_group_reachable;

    Time m_current_time{0};
    EventListener* m_event_listener{nullptr};
    PassengerID m_next_passenger_id{1};
};

}
