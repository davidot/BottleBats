#pragma once

#include "Types.h"
#include "stats/Listener.h"

namespace Elevated {

    class BuildingState {

        std::optional<Time> next_event_at() const;

        void add_request(Passenger passenger);
        void send_elevator(ElevatorID, Height target);
        void update_until(Time target_time);
    private:
        std::unordered_map<Height, std::vector<Passenger>> m_floors;
        std::vector<ElevatorState> m_elevators;
        std::unordered_map<ElevatorID, ElevatorState&> m_elevator_by_id;

        Time m_current_time;

#ifndef NDEBUG
        std::unordered_map<GroupID, std::unordered_set<Height>> m_group_reachable;
#endif

    };

}
