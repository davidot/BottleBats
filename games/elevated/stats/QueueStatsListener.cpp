#include "QueueStatsListener.h"
#include "../../../util/Assertions.h"

namespace Elevated {

void QueueStatsListener::on_request_created(Elevated::Time, Elevated::Passenger const& passenger)
{
    m_queue_per_floor[passenger.from].add();
    m_queue_per_group[passenger.group].add();
    m_total_waiting.add();
}

void QueueStatsListener::on_passenger_enter_elevator(Elevated::Time, Elevated::Passenger const& passenger, Elevated::ElevatorID)
{
    m_queue_per_floor[passenger.from].remove();
    ASSERT(m_queue_per_floor[passenger.from].current < m_queue_per_floor[passenger.from].max);

    m_queue_per_group[passenger.group].remove();
    ASSERT(m_queue_per_group[passenger.group].current < m_queue_per_group[passenger.group].max);

    m_total_waiting.remove();
    ASSERT(m_total_waiting.current < m_total_waiting.max);
}

uint64_t QueueStatsListener::max_floor_queue() const
{
    if (m_queue_per_floor.empty())
        return 0;

    return std::max_element(m_queue_per_floor.begin(), m_queue_per_floor.end(),
        [](auto const& lhs, auto const& rhs) {
        return lhs.second.max < rhs.second.max;
        })->second.max;
}

}
