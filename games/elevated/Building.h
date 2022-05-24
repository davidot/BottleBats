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

    std::optional<size_t> add_request(PassengerBlueprint passenger);
    bool send_elevator(ElevatorID, Height target);

    struct UpdateResult {
        enum class Type {
            DoorsOpened,
            DoorsClosed
        };
        Type type;
        ElevatorID id;
    };

    std::vector<UpdateResult> update_until(Time target_time);
    void transfer_passengers(ElevatorID id, ElevatorState::PassengerCallback const& callback = [](Passenger const&){ return true; });

    [[nodiscard]] std::vector<Passenger> const& passengers_at(Height) const;
    [[nodiscard]] ElevatorState const& elevator(ElevatorID) const;
    [[nodiscard]] size_t num_elevators() const { return m_elevators.size(); }
    [[nodiscard]] Time current_time() const { return m_current_time; }

    bool passengers_done();

    std::vector<Height> all_floors() const;

private:
    std::unordered_map<Height, std::vector<Passenger>> m_floors;
    std::vector<ElevatorState> m_elevators;
    std::vector<std::unordered_set<Height>> m_group_reachable;

    Time m_current_time{0};
    EventListener* m_event_listener{nullptr};
    PassengerID m_next_passenger_id{1};
};

}
