#include "Listener.h"
#include "../../../util/Assertions.h"

namespace Elevated {
    void EventDistributor::add_listener(std::shared_ptr<EventListener> listener) {
        ASSERT(listener.get() != this);
        ASSERT(std::find_if(m_listeners.begin(), m_listeners.end(), [&](std::shared_ptr<EventListener> const& l) {
            return l == listener;
        }) == m_listeners.end());
        m_listeners.emplace_back(std::move(listener));
    }

    bool EventDistributor::remove_listener(EventListener* listener_ptr) {
        ASSERT(listener_ptr != this);
        auto listener_or_end = std::find_if(m_listeners.begin(), m_listeners.end(), [&](std::shared_ptr<EventListener> const& listener) {
            return listener.get() == listener_ptr;
        });

        if (listener_or_end == m_listeners.end())
            return false;

        m_listeners.erase(listener_or_end);
        return true;
    }

    void EventDistributor::on_request_created(Time at, Passenger const& passenger) {
        for (auto& listener : m_listeners)
            listener->on_request_created(at, passenger);
    }

    void EventDistributor::on_passenger_enter_elevator(Time at, Passenger const& passenger,
                                                       ElevatorID id) {
        for (auto& listener : m_listeners)
            listener->on_passenger_enter_elevator(at, passenger, id);
    }

    void EventDistributor::on_passenger_leave_elevator(Time at, PassengerID id, Height height) {
        for (auto& listener : m_listeners)
            listener->on_passenger_leave_elevator(at, id, height);
    }

    void EventDistributor::on_elevator_opened_doors(Time at, ElevatorState const& elevator) {
        for (auto& listener : m_listeners)
            listener->on_elevator_opened_doors(at, elevator);
    }

    void EventDistributor::on_elevator_closed_doors(Time at, ElevatorState const& elevator) {
        for (auto& listener : m_listeners)
            listener->on_elevator_closed_doors(at, elevator);
    }

    void EventDistributor::on_elevator_set_target(Time at, Height new_target,
                                                  ElevatorState const& elevator) {
        for (auto& listener : m_listeners)
            listener->on_elevator_set_target(at, new_target, elevator);
    }

    void EventDistributor::on_elevator_stopped(Time at, Time duration, const ElevatorState &elevator) {
        for (auto& listener : m_listeners)
            listener->on_elevator_stopped(at, duration, elevator);
    }
}
