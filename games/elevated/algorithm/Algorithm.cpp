#include "Algorithm.h"
#include "../../../util/Assertions.h"

namespace Elevated {

ElevatedAlgorithm::ScenarioAccepted ElevatedAlgorithm::ScenarioAccepted::accepted()
{
    return {};
}

ElevatedAlgorithm::ScenarioAccepted ElevatedAlgorithm::ScenarioAccepted::failed(std::vector<std::string> messages)
{
    return {Type::Failed, std::move(messages)};
}
ElevatedAlgorithm::ScenarioAccepted ElevatedAlgorithm::ScenarioAccepted::rejected(std::vector<std::string> messages)
{
    return {Type::Rejected, std::move(messages)};
}

ElevatedAlgorithm::ScenarioAccepted::ScenarioAccepted() noexcept
    : type(Type::Accepted)
{
}

ElevatedAlgorithm::ScenarioAccepted::ScenarioAccepted(Type type_, std::vector<std::string> messages_)
    : type(type_)
    , messages(std::move(messages_))
{
}

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
    response.m_data = timer_at;
    return response;
}

AlgorithmResponse AlgorithmResponse::move_elevator_to(ElevatorID elevator_id, Height target) {
    AlgorithmResponse response{};
    response.m_type = Type::MoveElevator;
    response.m_data = ElevatorMove { elevator_id, target };
    return response;
}

AlgorithmResponse AlgorithmResponse::algorithm_failed(std::vector<std::string> messages) {
    AlgorithmResponse response{};
    response.m_type = Type::AlgorithmFailed;
    response.m_data = std::move(messages);
    return response;
}

AlgorithmResponse AlgorithmResponse::algorithm_misbehaved(std::vector<std::string> messages) {
    AlgorithmResponse response{};
    response.m_type = Type::AlgorithmMisbehaved;
    response.m_data = std::move(messages);
    return response;
}

Time AlgorithmResponse::timer_should_fire_at() const {
    ASSERT(m_type == Type::SetTimer);
    ASSERT(std::holds_alternative<Time>(m_data));
    return std::get<Time>(m_data);
}

ElevatorID AlgorithmResponse::elevator_to_move() const {
    ASSERT(m_type == Type::MoveElevator);
    ASSERT(std::holds_alternative<ElevatorMove>(m_data));
    return std::get<ElevatorMove>(m_data).id;
}

Height AlgorithmResponse::elevator_target() const {
    ASSERT(m_type == Type::MoveElevator);
    ASSERT(std::holds_alternative<ElevatorMove>(m_data));
    return std::get<ElevatorMove>(m_data).target;
}

std::vector<std::string> const& AlgorithmResponse::messages() const
{
    ASSERT(m_type == Type::AlgorithmFailed || m_type == Type::AlgorithmMisbehaved);
    ASSERT(std::holds_alternative<std::vector<std::string>>(m_data));
    return std::get<std::vector<std::string>>(m_data);
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
