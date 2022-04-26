#include "Algorithm.h"
#include "../../../util/Assertions.h"

namespace Elevated {

AlgorithmInput AlgorithmInput::new_request(Height at, size_t index) {
    AlgorithmInput input;
    input.m_type = Type::NewRequestMade;
    input.request_height = at;
    input.request_index = index;
    return input;
}

AlgorithmInput AlgorithmInput::elevator_closed_doors(ElevatorID id) {
    AlgorithmInput input;
    input.m_type = Type::ElevatorClosedDoors;
    input.m_elevator_id = id;
    return input;
}

AlgorithmInput AlgorithmInput::timer_fired() {
    AlgorithmInput input;
    input.m_type = Type::TimerFired;
    return input;
}

AlgorithmResponse AlgorithmResponse::set_timer_at(Time timer_at) {
    AlgorithmResponse response{};
    response.m_type = Type::SetTimer;
    response.m_timer_at = timer_at;
    return response;
}

AlgorithmResponse AlgorithmResponse::move_elevator_to(ElevatorID elevator_id, Height target) {
    AlgorithmResponse response{};
    response.m_type = Type::MoveElevator;
    response.m_elevator_id = elevator_id;
    response.m_target = target;
    return response;
}

Time AlgorithmResponse::timer_should_fire_at() const {
    ASSERT(m_type == Type::SetTimer);
    return m_timer_at;
}

ElevatorID AlgorithmResponse::elevator_to_move() const {
    ASSERT(m_type == Type::MoveElevator);
    return m_elevator_id;
}

Height AlgorithmResponse::elevator_target() const {
    ASSERT(m_type == Type::MoveElevator);
    return m_target;
}

ElevatorID AlgorithmInput::elevator_id() const {
    ASSERT(m_type == Type::ElevatorClosedDoors);
    return m_elevator_id;
}

Passenger const& AlgorithmInput::request(BuildingState const& building) const {
    ASSERT(m_type == Type::NewRequestMade);
    auto& queue = building.passengers_at(request_height);
    ASSERT(request_index < queue.size());
    return queue[request_index];
}

}
