#include "Building.h"
#include "../../util/Assertions.h"

namespace Elevated {

void BuildingState::update_until(Time target_time)
{
    ASSERT(target_time > m_current_time);
    ASSERT(target_time <= next_event_at().value_or(target_time));

    Time steps = target_time - m_current_time;

    std::vector<ElevatorID> elevators_closed_doors;

    for (auto& elevator : m_elevators) {
        if (auto result = elevator.update(steps); result != ElevatorState::ElevatorUpdateResult::Nothing) {
            ASSERT(m_floors.contains(elevator.height()));
            if (result == ElevatorState::ElevatorUpdateResult::DoorsOpened) {
                auto& floor_stopped_at = m_floors[elevator.height()];
                elevator.transfer_passengers(floor_stopped_at);
            } else {
                elevators_closed_doors.push_back(elevator.id);
            }
        }
    }
}


    void BuildingState::add_request(Passenger passenger)
    {
        ASSERT(m_floors.contains(passenger.from));
        ASSERT(m_floors.contains(passenger.to));
        ASSERT(m_group_reachable[passenger.group].contains(passenger.from));

        m_floors[passenger.from].push_back(passenger);
        // FIXME: Generate event!
    }

    void BuildingState::send_elevator(ElevatorID id, Height target)
    {
        auto it = m_elevator_by_id.find(id);
        ASSERT(it != m_elevator_by_id.end());
        auto& elevator = it->second;
//    auto old_target = elevator.target_height();
        elevator.set_target(target);
        // FIXME: Generate rollercoaster event!
    }

    std::optional<Time> BuildingState::next_event_at() const
    {
        std::optional<Time> first_time;
        for (auto& elevator : m_elevators) {
            auto time_or_none = elevator.time_until_next_event();
            if (!time_or_none.has_value())
                continue;

            Time next_value = m_current_time + time_or_none.value();

            if (!first_time.has_value() || first_time.value() > next_value)
                first_time = next_value;
        }

        return first_time;
    }


}
