#pragma once

#include "Listener.h"


template<typename ValueType>
struct CounterWithMax {
    ValueType current{};
    ValueType max{};

    void add() {
        if (++current > max)
            max = current;
    }

    void remove() {
        --current;
    }
};

namespace Elevated {
class QueueStatsListener : public EventListener {
public:
    void on_request_created(Time time, Passenger const& passenger) override;
    void on_passenger_enter_elevator(Time time, Passenger const& passenger, ElevatorID id) override;

    uint64_t max_floor_queue() const;

private:
    std::unordered_map<Height, CounterWithMax<uint64_t>> m_queue_per_floor;
    std::unordered_map<Height, CounterWithMax<uint64_t>> m_queue_per_group;
    CounterWithMax<uint64_t> m_total_waiting;
};


}
